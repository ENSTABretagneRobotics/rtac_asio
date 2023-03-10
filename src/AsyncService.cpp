/**
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <rtac_asio/AsyncService.h>
#include <functional>

namespace rtac { namespace asio {

AsyncService::AsyncService() :
    thread_(nullptr),
    isRunning_(false),
    timer_(service_)
{}

AsyncService::~AsyncService()
{
    this->stop();
}

void AsyncService::timer_callback(const ErrorCode& /*err*/) const
{
    //std::cout << "==== Worker timeout guard ====" << std::endl;
    timer_.expires_from_now(Millis(1000));
    timer_.async_wait(std::bind(&AsyncService::timer_callback, this, std::placeholders::_1));
}

void AsyncService::run()
{
    {
        std::unique_lock<std::mutex> lock(waitMutex_);
        if(isRunning_) {
            return;
        }
        isRunning_ = true;
        waiterWasNotified_ = true;
        //lock.unlock(); // ?
        waitStart_.notify_one();
    }
    try {
        //service_.reset(); // deprecated
        service_.restart();

        // Giving work to the service to prevent it from stopping right away
        timer_.cancel();
        this->timer_callback(ErrorCode());

        service_.run();
    }
    catch(...) {
        // This ensures isRunning_ is set to false if service::run() throws an
        // exception. The exception is rethrown immediately
        isRunning_ = false;
        throw;
    }
    {
        isRunning_ = false;
    }
}

bool AsyncService::is_running() const
{
    //std::lock_guard<std::mutex> lock(startMutex_); // necessary ?
    return isRunning_;
}

void AsyncService::start()
{
    std::lock_guard<std::mutex> lock(startMutex_);
    if(isRunning_) {
        return;
    }
    if(thread_) {
        service_.stop();
        thread_->join();
        thread_ = nullptr; // this is to force deletion of the old thread
    }

    // this construct allows to ensure thread is started and isRunning_ is set
    // to true before this function exits and release startMutex_. This
    // protects from deadlock if another call to start was made in the
    // meantime.
    std::unique_lock<std::mutex> waiterLock(waitMutex_);
    thread_ = std::make_unique<std::thread>(std::bind(&AsyncService::run, this));
    if(!thread_->joinable()) {
        throw std::runtime_error("rtac::asio::AsyncService : could not start working thread");
    }

    // using waiterWasNotified_ instead of isRunning_, because isRunning_ may
    // be reset to false too rapidly for the waiter to wakeup.
    waiterWasNotified_ = false;
    waitStart_.wait(waiterLock, [&]{ return waiterWasNotified_; });
}

void AsyncService::stop()
{
    std::lock_guard<std::mutex> lock(startMutex_);

    // thread MUST be joined in any case. Check of isRunning_ can be hurtfull
    //if(!isRunning_) {
    //    return;
    //}
    
    service_.stop();
    if(thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_ = nullptr;
}

void AsyncService::post(const std::function<void()>& function)
{
    boost::asio::post(service_, function);
}

} //namespace asio
} //namespace rtac

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



#ifndef _DEF_RTAC_ASIO_STREAM_READER_H_
#define _DEF_RTAC_ASIO_STREAM_READER_H_

#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include <rtac_asio/AsyncService.h>
#include <rtac_asio/StreamInterface.h>

namespace rtac { namespace asio {

class StreamReader
{
    public:

    using Ptr      = std::shared_ptr<StreamReader>;
    using ConstPtr = std::shared_ptr<const StreamReader>;

    using ErrorCode = StreamInterface::ErrorCode;
    using Callback  = StreamInterface::Callback;

    using Timer  = boost::asio::deadline_timer;
    using Millis = boost::posix_time::milliseconds;

    protected:

    StreamInterface::ConstPtr stream_;

    unsigned int readCounter_;
    unsigned int readId_;
    std::size_t  requestedSize_;
    std::size_t  processed_;
    uint8_t*     dst_;
    Callback     callback_;

    Timer timer_;

    // synchronization for synchronous read
    std::mutex              mutex_;
    std::condition_variable waiter_;
    bool                    waiterNotified_;

    StreamReader(StreamInterface::ConstPtr stream);

    void async_read_continue(unsigned int readId,
                             const ErrorCode& err, std::size_t readCount);
    void timeout_reached(unsigned int readId, const ErrorCode& err);
    void read_callback(const ErrorCode& err, std::size_t readCount);

    public:

    ~StreamReader();

    static Ptr Create(StreamInterface::ConstPtr stream);

    void async_read_some(std::size_t count, uint8_t* data,
                         Callback callback);

    void async_read(std::size_t count, uint8_t* data, Callback callback,
                    unsigned int timeoutMillis = 0);

    std::size_t read(std::size_t count, uint8_t* data,
                     int64_t timeoutMillis = 0);
};

} //namespace asio
} //namespace rtac

#endif //_DEF_RTAC_ASIO_STREAM_READER_H_

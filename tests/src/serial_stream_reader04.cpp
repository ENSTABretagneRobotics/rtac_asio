#include <iostream>
#include <functional>
using namespace std;
using namespace std::placeholders;

#include <rtac_asio/AsyncService.h>
#include <rtac_asio/SerialStream.h>
#include <rtac_asio/StreamReader.h>
using namespace rtac::asio;

std::string msg = "Hello there !\n";

void write_callback(const SerialStream::ErrorCode& err,
                    std::size_t writeCount)
{
    std::cout << "Wrote data (" << writeCount << " bytes)." << std::endl;
}

void read_callback(StreamReader::Ptr stream,
                   std::string* data,
                   const SerialStream::ErrorCode& err,
                   std::size_t count)
{
    if(count > 0) {
        std::cout << "Got data (" << count << " bytes) : '";
        for(int i = 0; i < count; i++) {
            std::cout << (*data)[i];
        }
        std::cout << "'" << std::endl;
    }
    else {
        std::cout << "Got no data (timeout reached ?)" << std::endl;
    }
    stream->async_read_until(data->size(), (uint8_t*)data->c_str(), '\n',
                             std::bind(&read_callback, stream, data, _1, _2));
                             //std::bind(&read_callback, stream, data, _1, _2), 1000);
}

int main()
{
    std::string data(1024, '\0');

    auto service = AsyncService::Create();
    auto serial = SerialStream::Create(service, "/dev/ttyACM0");
    auto stream = StreamReader::Create(serial);

    stream->async_read_until(data.size(), (uint8_t*)data.c_str(), '\n',
                             std::bind(&read_callback, stream, &data, _1, _2));

    service->start();
    std::cout << "Started" << std::endl;
    
    while(1) {
        getchar();
        serial->async_write_some(msg.size(),
                                 (const uint8_t*)msg.c_str(),
                                 &write_callback);
        std::cout << "Service running ? : " << !service->stopped() << std::endl;
    }

    service->stop();

    return 0;
}

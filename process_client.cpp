#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#include <iceoryx_posh/popo/client.hpp>
#include <iceoryx_posh/runtime/posh_runtime.hpp>
#include <iostream>

int main()
{
    constexpr char APP_NAME[] = "client-basic";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    //! TODO: modify the template parameter to Arrow (the first is the request, the second is the response)
    iox::popo::Client<std::pair<uint64_t, uint64_t>, std::pair<uint64_t, uint64_t>> client({"Example", "CURD", "Add"});

    uint64_t a = 0, b = 1;
    int64_t rsid = 0;
    int16_t ersid = rsid;
    while (!iox::posix::hasTerminationRequested())
    {
        client.loan().and_then([&](auto &request)
                               {
            request.getRequestHeader().setSequenceId(rsid);
            ersid = rsid;
            rsid += 1;
            request->first = a;
            request->second = b;
            std::cout<<"[Client] send: "<< a <<", "<<b<<std::endl;
            request.send().or_else(
                [&](auto& error){std::cout<<"[Client] send faild !\n";}); })
            .or_else([](auto &error)
                     { std::cout << "[Client] send faild !\n"; });
        constexpr std::chrono::milliseconds DELAY_TIME{150U};
        std::this_thread::sleep_for(DELAY_TIME);

        while (client.take().and_then([&](const auto& response) {
            auto redsid = response.getResponseHeader().getSequenceId();
            if (redsid == ersid) 
            {
                a = b;
                b = response->first;
                std::cout << "[Client] Got Response : " << b << std::endl;
            }
            else
            {
                std::cout << "[Client] Got Response with outdated sequence ID! Expected = " << ersid
                          << "; Actual = " << redsid << "! -> skip" << std::endl;
            }
        }))
        {
        };
        constexpr std::chrono::milliseconds SLEEP_TIME{950U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    return 0;
}
#include "translation_data_type.hpp"
#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#include <iceoryx_posh/popo/server.hpp>
#include <iceoryx_posh/runtime/posh_runtime.hpp>

#include <iostream>

int main()
{
    constexpr char APP_NAME[] = "server-basic";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    //! TODO: modify the template parameter to Arrow (the first is the request, the second is the response)
    iox::popo::Server<TranslationRequest, TranslationResponse> server({"Example", "CURD", "Add"});

    while (!iox::posix::hasTerminationRequested())
    {
        server.take().and_then([&](const auto& request) {
            std::cout << APP_NAME << "[Server] Got Request: " << request->data.size() << std::endl;

            server.loan(request)
                .and_then([&](auto& response) {
                    response->data = request->data;
                    std::cout << APP_NAME << "[Server] Send Response: " << response->data.size() << std::endl;
                    response.send().or_else(
                        [&](auto& error) { std::cout << "[Server] Could not send Response! Error: " << error << std::endl; });
                })
                .or_else([&](auto& error) {
                    std::cout << APP_NAME << "[Server] Could not allocate Response! Error: " << error << std::endl;
                });
        });

        constexpr std::chrono::milliseconds SLEEP_TIME{100U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    return 0;
}
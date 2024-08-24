cmake_minimum_required(VERSION 3.16)
project(process)

include(GNUInstallDirs)


find_package(iceoryx_posh REQUIRED)
find_package(iceoryx_hoofs REQUIRED)

include(IceoryxPlatform)


add_executable(
    client
    process_client.cpp
)


target_link_libraries(
    client
    PRIVATE
    iceoryx_hoofs::iceoryx_hoofs
    iceoryx_posh::iceoryx_posh
)

add_executable(
    server
    process_server.cpp
)


target_link_libraries(
    server
    PRIVATE
    iceoryx_hoofs::iceoryx_hoofs
    iceoryx_posh::iceoryx_posh
)

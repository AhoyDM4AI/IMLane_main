cmake_minimum_required(VERSION 3.16)
project(process_boost)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost REQUIRED)


add_executable(server process_server.cpp)
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server pthread rt ${Boost_LIBRARIES} )

add_executable(client process_client.cpp)
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(client pthread rt ${Boost_LIBRARIES} )
cmake_minimum_required(VERSION 3.16)
project(process_boost)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost REQUIRED)
find_package(Arrow REQUIRED)

#message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
#message(STATUS "Found Boost: ${Boost_LIBRARIES}")
#message(STATUS "Found Arrow: ${Arrow_INCLUDE_DIRS}")
#message(STATUS "Found Arrow: ${Arrow_LIBRARIES}")



add_executable(server process_server.cpp)
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared)

add_executable(client process_client.cpp)
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(client pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared)

add_executable(arrow_demo arrow_demo.cpp)
# target_include_directories(arrow_demo PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(arrow_demo PRIVATE Arrow::arrow_shared)
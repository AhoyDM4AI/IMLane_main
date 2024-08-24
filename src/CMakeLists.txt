cmake_minimum_required(VERSION 3.16)
project(process_boost)

set(CMAKE_CXX_STANDARD 17)
set(PYTHON_INCLUDE_DIRS /root/miniconda3/envs/duckdb/include/python3.9)
set(PYTHON_LIBRARIES /root/miniconda3/envs/duckdb/lib/libpython3.9.so)
set(ARROW_PYTHON_INCLUDE_DIR /root/miniconda3/envs/duckdb/lib/python3.9/site-packages/pyarrow/include)
set(ARROW_PYTHON_LIB_DIR /root/miniconda3/envs/duckdb/lib/python3.9/site-packages/pyarrow)

find_package(Boost REQUIRED)
find_package(Arrow REQUIRED)

link_directories(${ARROW_PYTHON_LIB_DIR})


add_executable(server process_server.cpp)
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} ${ARROW_PYTHON_INCLUDE_DIR})
target_link_libraries(server pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared ${PYTHON_LIBRARIES} arrow_python)

add_executable(client process_client.cpp)
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(client pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared)

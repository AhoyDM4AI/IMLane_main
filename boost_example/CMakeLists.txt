cmake_minimum_required(VERSION 3.16)
project(process_boost)

set(CMAKE_CXX_STANDARD 14)
set(pybind11_DIR /root/workspace/duckdb/tools/pythonpkg/.eggs/pybind11-2.13.1-py3.9.egg/pybind11/share/cmake/pybind11)
set(PYTHON_INCLUDE_DIRS /root/miniconda3/envs/duckdb/include/python3.9)
set(PYTHON_LIBRARIES /root/miniconda3/envs/duckdb/lib)
set(ARROW_PYTHON_INCLUDE_DIR /root/miniconda3/envs/duckdb/lib/python3.9/site-packages/pyarrow/include)
set(ARROW_PYTHON_LIB_DIR /root/miniconda3/envs/duckdb/lib/python3.9/site-packages/pyarrow)
# set(ARROW_PYTHON_LIB_DIR /root/lib)

find_package(Boost REQUIRED)
find_package(Arrow REQUIRED)
find_package(pybind11)

link_directories(${ARROW_PYTHON_LIB_DIR})

add_executable(server process_server.cpp)
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS} ${pybind11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS} ${ARROW_PYTHON_INCLUDE_DIR})
target_link_libraries(server pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared pybind11::embed ${PYTHON_LIBRARIES} arrow_python)

add_executable(client process_client.cpp)
target_include_directories(client PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(client pthread rt ${Boost_LIBRARIES} Arrow::arrow_shared)

add_executable(arrow_demo arrow_demo.cpp)
# target_include_directories(arrow_demo PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(arrow_demo PRIVATE Arrow::arrow_shared)
cmake_minimum_required(VERSION 3.16)
project(process_boost)

set(CMAKE_CXX_STANDARD 14)
set(pybind11_DIR /workspace/duckdb/tools/pythonpkg/.eggs/pybind11-2.13.1-py3.9.egg/pybind11/share/cmake/pybind11)
set(ARROW_PYTHON_INCLUDE_DIR /usr/local/lib/python3.8/dist-packages/pyarrow/include)
set(ARROW_PYTHON_LIB_DIR /usr/local/lib/python3.8/dist-packages/pyarrow)

find_package(Boost REQUIRED)
find_package(Arrow REQUIRED)
find_package(PythonLibs 3 REQUIRED)
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

add_executable(py_warp_example py_warp_example.cpp)
target_include_directories(py_warp_example PRIVATE   ${PYTHON_INCLUDE_DIRS} ${ARROW_PYTHON_INCLUDE_DIR})
target_link_libraries(py_warp_example  Arrow::arrow_shared ${PYTHON_LIBRARIES} arrow_python)
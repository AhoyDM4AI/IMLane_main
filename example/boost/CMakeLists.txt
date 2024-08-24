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

add_executable(c_array2c_plus_table c_array2c_plus_table.cpp)
target_include_directories(c_array2c_plus_table PRIVATE   ${Boost_INCLUDE_DIRS} ${Arrow_INCLUDE_DIRS})
target_link_libraries(c_array2c_plus_table ${Boost_LIBRARIES} Arrow::arrow_shared ${Arrow_LIBRARIES} rt pthread)

add_executable(arrow_demo arrow_demo.cpp)
target_link_libraries(arrow_demo PRIVATE Arrow::arrow_shared)

add_executable(py_warp_example py_warp_example.cpp)
target_include_directories(py_warp_example PRIVATE   ${PYTHON_INCLUDE_DIRS} ${ARROW_PYTHON_INCLUDE_DIR})
target_link_libraries(py_warp_example  Arrow::arrow_shared ${PYTHON_LIBRARIES} arrow_python)

add_executable(test1 test1.cpp)
target_include_directories(test1 PRIVATE   ${Boost_INCLUDE_DIRS})
target_link_libraries(test1 ${Boost_LIBRARIES} rt pthread)

add_executable(test2 test2.cpp)
target_include_directories(test2 PRIVATE   ${Boost_INCLUDE_DIRS})
target_link_libraries(test2 ${Boost_LIBRARIES} rt pthread)
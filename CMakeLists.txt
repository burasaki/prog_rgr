cmake_minimum_required(VERSION 3.10)
project(BMPRGR C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(main.exe main.c script.c RGR.h)

enable_testing()

find_package(cmocka REQUIRED)

include_directories(${CMOCKA_INCLUDE_DIR})
    
add_executable(test.exe tests/test.c script.c RGR.h)
    
target_link_libraries(test.exe ${CMOCKA_LIBRARIES})
    
add_test(NAME CMockaTests COMMAND test.exe)
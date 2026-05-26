cmake_minimum_required(VERSION 3.10)
project(BMPRGR C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(main.exe main.c script.c RGR.h)
configure_file(config.txt config.txt COPYONLY)



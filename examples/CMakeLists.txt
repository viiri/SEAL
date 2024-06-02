cmake_minimum_required(VERSION 3.20)
project(seal-examples)

set(CMAKE_C_STANDARD 99)

foreach(i RANGE 1 6)
    add_executable("example${i}")
    target_sources("example${i}" PRIVATE "example${i}.c")
    target_link_libraries("example${i}" seal)
endforeach()

if(NOT MSVC)
    target_link_libraries("example6" m)
endif()

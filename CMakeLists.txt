cmake_minimum_required(VERSION 3.20)
project("SEAL" VERSION 1.0.7 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

option(SEAL_SHARED_LIBS "Build using shared libraries" ON)
option(SEAL_BUILD_MP "Build module player" ON)
option(SEAL_BUILD_EXAMPLES "Build examples" ON)
option(SEAL_INSTALL "Generate target for install" ${is_top_level})

if(DEFINED SEAL_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${SEAL_SHARED_LIBS}")
endif()

add_library(seal)
add_library(SEAL::seal ALIAS seal)

target_include_directories(seal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(seal PROPERTIES
    PUBLIC_HEADER include/audio.h
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION})

target_sources(seal PRIVATE
    src/audio.c
    src/iofile.c
    src/mixdrv.c
    src/modeng.c
    src/modfile.c
    src/mtmfile.c
    src/nondrv.c
    src/s3mfile.c
    src/wavfile.c
    src/xmfile.c
)

find_package(SDL2)

if(SDL2_FOUND)
    target_compile_definitions(seal PUBLIC "__SDL2__")
    target_sources(seal PRIVATE src/sdldrv.c)
    target_link_libraries(seal PRIVATE SDL2::SDL2)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(seal PUBLIC "__LINUX__")
endif()

if(WIN32)
    target_compile_definitions(seal PUBLIC "__WINDOWS__")
    if(BUILD_SHARED_LIBS)
        set_target_properties(seal PROPERTIES OUTPUT_NAME audiow32)
        target_sources(seal PRIVATE src/audiow32.def)
    endif()
endif()

if(SEAL_BUILD_MP)
    add_executable(mp)
    target_sources(mp PRIVATE src/mp.c)
    target_link_libraries(mp PRIVATE seal)
endif()

if(SEAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(SEAL_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    install(TARGETS seal mp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

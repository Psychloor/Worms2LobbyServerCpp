cmake_minimum_required(VERSION 3.31)

# vcpkg integration
if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif ()

project(WormsServer
        VERSION 1.2.2
        LANGUAGES CXX
        DESCRIPTION "Worms game server"
)

# Build configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensures -std=c++23 instead of -std=gnu++23

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif ()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Multithreading support
if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    if (MSVC)
        add_definitions(-D_WIN32_WINNT=0x0601 -D_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif ()
else ()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif ()

set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nation.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/packet_code.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/worms_packet.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/session_info.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/packet_flags.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/windows_1251.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/database.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/user.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/room.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/game.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/user_session.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/packet_handler.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/server.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/string_utils.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/windows_1252.hpp
)

set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/worms_packet.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/database.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/user.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/room.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/game.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/user_session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/packet_handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session_info.cpp
)

# Dependencies
find_package(asio CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
        PacketIO
        GIT_REPOSITORY https://github.com/Psychloor/PacketIO.git
        GIT_TAG main
)
FetchContent_MakeAvailable(PacketIO)

# Configure spdlog options
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "Use external fmt library instead of bundled" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Build example" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "Generate the install target" FORCE)
find_package(spdlog REQUIRED)


# Optimization flags for non-MSVC compilers
if (NOT MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2)
    endif ()
endif ()

# Main executable
add_executable(${PROJECT_NAME} main.cpp
        ${HEADERS}
        ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        $<$<NOT:$<PLATFORM_ID:Windows>>:Threads::Threads>
        asio::asio
        spdlog::spdlog
        unofficial::concurrentqueue::concurrentqueue
        PacketIO::PacketIO
)

# Installation rules
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS}
        DESTINATION include/${PROJECT_NAME}
)


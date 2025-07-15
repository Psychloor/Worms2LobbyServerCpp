cmake_minimum_required(VERSION 3.31)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

project(WormsServer
        VERSION 1.0.0
        LANGUAGES CXX
        DESCRIPTION "Worms game server"
)

# Build configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensures -std=c++23 instead of -std=gnu++23

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")


# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Windows-specific definitions
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later
endif()


set(LIBHEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/packet_buffers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/resettable_timeout.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/framed_packet_reader.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/framed_deserialization_result.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/concurrentqueue.hpp
)

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
)

# Dependencies
set(Boost_DIR "C:/Boost" CACHE PATH "Path to Boost installation")
file(TO_CMAKE_PATH "${Boost_DIR}" Boost_DIR)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

# spdlog setup
add_subdirectory(third_party/spdlog)
set(SPDLOG_USE_STD_FORMAT ON)

# Optimization flags for non-MSVC compilers
if(NOT MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(-O2)
    endif()
endif()


# Library and executable targets
add_library(header_only INTERFACE ${LIBHEADERS})
target_include_directories(header_only INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only
)

add_executable(${PROJECT_NAME} main.cpp
        ${HEADERS}
        ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        header_only
        spdlog::spdlog
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

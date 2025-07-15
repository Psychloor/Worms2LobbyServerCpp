cmake_minimum_required(VERSION 3.31)

project(WormsServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later

set(LIBHEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/packet_buffers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/resettable_timeout.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only/framed_packet_reader.hpp
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

find_package(Boost REQUIRED)

set(SPDLOG_USE_STD_FORMAT ON)
add_subdirectory(third_party/spdlog)


if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_options(-O2 -g)
endif ()


add_library(header_only INTERFACE ${LIBHEADERS})
target_include_directories(header_only INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/header_only)


add_executable(${PROJECT_NAME} main.cpp
        ${HEADERS}
        ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include

)

target_link_libraries(${PROJECT_NAME} PRIVATE
        header_only
        spdlog::spdlog
)
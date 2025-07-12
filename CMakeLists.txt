cmake_minimum_required(VERSION 3.31)
project(WormsServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later

set(ICU_ROOT "C:/CppLibraries/ICU")

set(HEADERS
        include/nation.hpp
        include/packet_code.hpp
        include/worms_packet.hpp
        include/session_info.hpp
        include/packet_flags.hpp
        include/windows_1251.hpp
        include/database.hpp
        include/user.hpp
        include/room.hpp
        include/game.hpp
        include/packet_buffers.hpp
)

set(SOURCES
        src/worms_packet.cpp
        src/database.cpp
        src/user.cpp
        src/room.cpp
        src/game.cpp
)

find_package(Boost REQUIRED)

find_package(ICU COMPONENTS uc i18n data REQUIRED)

add_executable(${PROJECT_NAME} main.cpp
        ${HEADERS}
        ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${ICU_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ICU::uc
        ICU::i18n
        ICU::data
)
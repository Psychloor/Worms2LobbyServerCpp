cmake_minimum_required(VERSION 3.31)
project(WormsServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ICU_ROOT "C:/CppLibraries/ICU")

set(HEADERS
        include/nation.hpp
        include/packet_code.hpp
        include/worms_packet.hpp
        include/session_info.hpp
        include/packet_flags.hpp
        include/windows_1251.hpp
        include/database.hpp
)

set(SOURCES
        src/worms_packet.cpp
        src/database.cpp
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

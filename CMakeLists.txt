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

if (MSVC)
    if (NOT CMAKE_GENERATOR STREQUAL "Ninja")
        add_definitions(/MP)    # parallelize each target, unless Ninja is the generator
    endif ()
endif ()

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif ()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Multithreading support
if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    if (MSVC)
        add_definitions(-D_WIN32_WINNT=0x0A00 -D_CRT_SECURE_NO_WARNINGS)
    endif ()
else ()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif ()

file(
        GLOB_RECURSE HEADER_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/include/*.hpp
)
file(
        GLOB_RECURSE SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# Dependencies
find_package(asio CONFIG REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)

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

# Main executable
add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_sources(${PROJECT_NAME}
        PRIVATE
        main.cpp
        ${SOURCE_FILES}

        PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include/
        FILES
        ${HEADER_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        $<$<NOT:$<PLATFORM_ID:Windows>>:Threads::Threads>
        asio::asio
        spdlog::spdlog_header_only
        concurrentqueue::concurrentqueue
        PacketIO::PacketIO
)

include(CheckCXXSourceCompiles)

# Test for standard <coroutine> support and co_await
set(CODE_STD_CORO "
#include <coroutine>
struct awaitable {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};
struct task {
  struct promise_type {
    task get_return_value() noexcept { return {}; }
    task get_return_object() noexcept { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() {}
  };
};
task foo() { co_await awaitable{}; co_return; }
int main() { (void)foo(); }
")

check_cxx_source_compiles("${CODE_STD_CORO}" HAS_STD_CORO)

# If not, try experimental
if (NOT HAS_STD_CORO)
    set(CODE_EXP_CORO "
#include <experimental/coroutine>
namespace stdx = std::experimental;
struct awaitable {
  bool await_ready() const noexcept { return true; }
  void await_suspend(stdx::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};
struct task {
  struct promise_type {
    task get_return_value() noexcept { return {}; }
    task get_return_object() noexcept { return {}; }
    stdx::suspend_never initial_suspend() noexcept { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() {}
  };
};
task foo() { co_await awaitable{}; co_return; }
int main() { (void)foo(); }
")
    check_cxx_source_compiles("${CODE_EXP_CORO}" HAS_EXP_CORO)
endif()

# Apply definitions to your target(s) only when supported
if (HAS_STD_CORO)
    message(STATUS "Coroutines: std::coroutine detected")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ASIO_HAS_CO_AWAIT
            ASIO_HAS_STD_COROUTINE
    )
elseif (HAS_EXP_CORO)
    message(STATUS "Coroutines: experimental coroutine detected")
    # For older toolchains using experimental coroutines, ASIO_HAS_CO_AWAIT may still work,
    # but ASIO_HAS_STD_COROUTINE should NOT be defined.
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ASIO_HAS_CO_AWAIT
    )
else()
    message(STATUS "Coroutines not detected; building without co_await support")
endif()


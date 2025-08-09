#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <vector>
#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "server.hpp"
#include "user_session.hpp"


void InitializeLogging() {
    spdlog::init_thread_pool(8192, 1); // queue size, number of threads
    const auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    const auto fileSink    = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/worms_server.log", 0, 0, true);

    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
    const auto logger = std::make_shared<spdlog::async_logger>("Worms Server", sinks.begin(), sinks.end(),
        // use global thread pool
        spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::err);
    spdlog::flush_every(std::chrono::seconds(10));
    spdlog::cfg::load_env_levels();

    spdlog::apply_logger_env_levels(logger);

    spdlog::set_default_logger(logger);


    // Make sure to flush when the program exits
    std::atexit([]() { spdlog::shutdown(); });
}

bool ParseCommandLineArguments(
    const int argc, char** argv, uint16_t& port, size_t& maxConnections, size_t& maxThreads) {
    auto args = std::vector<std::string>(argv, argv + argc);
    for (const auto argsSlide = std::ranges::slide_view(args, 2); const auto& arg : argsSlide) {
        if (arg[0] == "-p" || arg[0] == "--port") {
            port = static_cast<uint16_t>(std::stoi(arg[1]));
            if (port <= 1024) {
                std::cerr << "Invalid port number, defaulting to 17000\n";
                port = 17000;
            }
        }

        if (arg[0] == "-c" || arg[0] == "--connections") {
            maxConnections = std::stoi(arg[1]);
            if (maxConnections < 1) {
                std::cerr << "Invalid connection count, defaulting to 1000\n";
                maxConnections = 1000;
            }
        }

        if (arg[0] == "-t" || arg[0] == "--threads") {
            maxThreads = std::stoi(arg[1]);
            if (maxThreads < 1) {
                std::cerr << "Invalid thread count, defaulting to " << std::thread::hardware_concurrency() << "\n";
                maxThreads = std::thread::hardware_concurrency();
            } else if (maxThreads > std::thread::hardware_concurrency()) {
                std::cerr << "Thread count cannot be higher than the number of "
                             "cores, defaulting to "
                          << std::thread::hardware_concurrency() << "\n";
                maxThreads = std::thread::hardware_concurrency();
            }
        }

        if (arg[0] == "-h" || arg[0] == "--help") {
            std::cout << "Usage: worms_server [options]\n"
                      << "Options:\n"
                      << "  -p, --port <port>			Port to listen on "
                         "(default: 17000)\n"
                      << "  -c, --connections <count> Maximum number of "
                         "connections (default: 10 000)\n"
                      << "  -t, --threads <count>		Maximum number of "
                         "threads (default: "
                      << std::thread::hardware_concurrency() << ")\n"
                      << "  -h, --help				Print this help message\n"
                      << std::endl;
            return true;
        }
    }
    return false;
}

int main(const int argc, char** argv) {
    uint16_t port          = 17000;
    size_t maxConnections = 10000;
    size_t maxThreads     = std::thread::hardware_concurrency();

    InitializeLogging();

    if (ParseCommandLineArguments(argc, argv, port, maxConnections, maxThreads)) {
        return 0;
    }

    try {
        worms_server::Server server(port, maxConnections);
        server.run(maxThreads);
    } catch (const std::exception& e) {
        spdlog::error("Fatal: {}", e.what());
        return 1;
    }

    return 0;
}

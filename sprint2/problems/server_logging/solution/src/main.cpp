#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <boost/asio/signal_set.hpp>
#include <filesystem>

#include <boost/log/trivial.hpp>
#include <string_view>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace {

// Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

}  // namespace

int main(int argc, const char* argv[]) {
    logware::InitLogger();

    if (argc != 3) {
        BOOST_LOG_TRIVIAL(error) << logware::CreateLogMessage("Usage: game_server <game-config-json>"sv,
                                                              logware::ExitCodeLogData(EXIT_FAILURE));
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        //model::Game game = json_loader::LoadGame("/home/stanislav/cpp-backend/sprint2/problems/static_content/solution/data/config.json");
        fs::path path_to_static_content{argv[2]};
        //fs::path path_to_static_content{"/home/stanislav/cpp-backend/sprint2/problems/static_content/solution/static"};

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                BOOST_LOG_TRIVIAL(info) << logware::CreateLogMessage("server exited"sv,
                                                                     logware::ExitCodeLogData(0));
                ioc.stop();
            }
        });
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, path_to_static_content};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        BOOST_LOG_TRIVIAL(info) << logware::CreateLogMessage("Server has started..."sv,
                                                             logware::ServerAddressLogData(address.to_string(), port));
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error) << logware::CreateLogMessage("error"sv,
                                                              logware::ExceptionLogData(EXIT_FAILURE, "Server down"sv, ex.what()));
        return EXIT_FAILURE;
    }
}

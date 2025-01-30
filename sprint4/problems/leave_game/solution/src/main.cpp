#include "sdk.h"

#include "magic_defs.h"
#include "ticker.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <thread>
#include <mutex>

#include "application.h"
#include "request_handler.h"
#include "logger.h"

using namespace std::literals;
using namespace json_loader;
using namespace json_logger;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace logging = boost::log;

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};
//GAME_DB_URL=postgres://postgres:Passwd@localhost:30432/game_db1

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
    try {

        // 1. Загружаем карту из файла и построить модель игры
        auto args = options::ParseCommandLine(argc, argv);
        if(!args.has_value()){
            std::cout << "Can't parsing program options" << std::endl;
            return EXIT_FAILURE;
        }

        app::AppConfig config;
        if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
            config.db_url = url;
        } else {
            throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
        }

        app::Application app(*args, config);
        app.RestoreBackUpData();

        // 2. Инициализируем io_context
        const unsigned num_threads = 1;
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
		// Подписываемся на сигналы и при их получении завершаем работу сервера
		net::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
			if (!ec) {
				ioc.stop();
			}
		});

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        if(args->tick_period){
            auto period = static_cast<std::chrono::milliseconds>(args->tick_period);
            auto ticker = std::make_shared<Ticker>(api_strand, period,
                                                   [&app](std::chrono::milliseconds delta) { app.Tick(delta); }
            );
            ticker->Start();
        }

        const auto address = net::ip::make_address(ServerParam::ADDR);
        constexpr net::ip::port_type port = ServerParam::PORT;

        auto endpoint = net::ip::tcp::endpoint{address, port};

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        //http_handler::RequestHandler handler{game, doc_root, api_strand};
        auto handler = std::make_shared<http_handler::RequestHandler>(app, api_strand);

        // Оборачиваем его в логирующий декоратор
        http_handler::LoggingRequestHandler logging_handler{
                [handler, endpoint]( auto&& req, auto&& send) {
                    // Обрабатываем запрос
                    (*handler)(endpoint, std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
                }};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        json_logger::SetupLogger();
        json::value start_data{{"port"s, ServerParam::PORT}, {"address", ServerParam::ADDR}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, start_data) << "server started"sv;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        app.BackUpData();

        json::value finish_data{{"code"s, 0}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, finish_data) << "server exited"sv;

    } catch (const std::exception& ex) {
        json::value finish_data{{"code"s, 1}, {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, finish_data) << "server exited"sv;

        return EXIT_FAILURE;
    }


}

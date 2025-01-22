#include "magic_defs.h"
#include "http_server.h"
#include "logger.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {
    using namespace json_logger;
    namespace json = boost::json;
    namespace logging = boost::log;

	void ReportError(beast::error_code ec, std::string_view what) {
        json::value error_data{{"code", ec.value()}, {"text", ec.message()}, {"where", what}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, error_data) << "error"sv;
	}

	void SessionBase::Read() {
		using namespace std::literals;
		// Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
		request_ = {};
		stream_.expires_after(30s);
		// Считываем request_ из stream_, используя buffer_ для хранения считанных данных
		http::async_read(stream_, buffer_, request_,
							// По окончании операции будет вызван метод OnRead
							beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
	}

	void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
		using namespace std::literals;
		if (ec == http::error::end_of_stream) {
			// Нормальная ситуация - клиент закрыл соединение
			return Close();
		}
		if (ec) {
			return ReportError(ec, ServerAction::READ);
		}
		HandleRequest(std::move(request_));
	}

	void SessionBase::Close() {
		beast::error_code ec;
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
	}

	void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
		if (ec) {
			return ReportError(ec, ServerAction::WRITE);
		}

		if (close) {
			// Семантика ответа требует закрыть соединение
			return Close();
		}

		// Считываем следующий запрос
		Read();
	}

	void SessionBase::Run() {
		// Вызываем метод Read, используя executor объекта stream_.
		// Таким образом вся работа со stream_ будет выполняться, используя его executor
		net::dispatch(stream_.get_executor(),
					  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
	}
} // namespace http_server
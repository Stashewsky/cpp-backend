#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "magic_defs.h"
#include "http_server.h"
#include "token.h"
#include "response.h"
#include "api_request.h"
#include "static_request.h"
#include "model.h"
#include "tools.h"
#include "logger.h"

#include <iostream>
#include <string_view>
#include <string>
#include <boost/json.hpp>
#include <functional>
#include <map>
#include <variant>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>

namespace http_handler {
    namespace beast = boost::beast;
    namespace json = boost::json;
    namespace logging = boost::log;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace sys = boost::system;
    namespace fs = std::filesystem;
    using namespace std::literals;
    using namespace std::chrono;
    using namespace json_logger;


    class SyncWriteOStreamAdapter {
    public:
        explicit SyncWriteOStreamAdapter(std::ostream& os)
                : os_{os} {
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs, sys::error_code& ec) {
            const size_t total_size = net::buffer_size(cbs);
            if (total_size == 0) {
                ec = {};
                return 0;
            }
            size_t bytes_written = 0;
            for (const auto& cb : cbs) {
                const size_t size = cb.size();
                const char* const data = reinterpret_cast<const char*>(cb.data());
                if (size > 0) {
                    if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                        ec = make_error_code(boost::system::errc::io_error);
                        return bytes_written;
                    }
                    bytes_written += size;
                }
            }
            ec = {};
            return bytes_written;
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs) {
            sys::error_code ec;
            const size_t bytes_written = write_some(cbs, ec);
            if (ec) {
                throw std::runtime_error("Failed to write");
            }
            return bytes_written;
        }

    private:
        std::ostream& os_;
    };

    template<class SomeRequestHandler>
    class LoggingRequestHandler {
        template <typename Body, typename Allocator, typename Send>
        static void LogRequest(net::ip::tcp::endpoint&& endpoint, http::request<Body, http::basic_fields<Allocator>>&& req) {
            json::value request_data{{"ip", endpoint.address().to_string()},
                                     {"URI", std::string(req.target())},
                                     {"method", req.method_string()}};
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, request_data)
                                    << "request received"sv;
        }

        template <typename Func, typename Resp>
        static void LogResponse(Func send, Resp resp) {
            std::string cont_type;
            resp.count("Content-Type") ? cont_type = resp.at("Content-Type") : cont_type = "null";
            auto start = steady_clock::now();
            send(resp);
            auto finish = steady_clock::now();
            int resp_time = duration_cast<milliseconds>(finish-start).count();
            json::value response_data{{"response_time", resp_time},
                                      {"code", resp.result_int()},
                                      {"content_type", cont_type}};
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, response_data)
                                    << "response sent"sv;
        }
    public:
        explicit LoggingRequestHandler(SomeRequestHandler handler) : decorated_(handler){}

        template <typename Body, typename Allocator, typename Send>
        void operator()( http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            decorated_( std::move(req), std::move(send));
        }

    private:
        SomeRequestHandler decorated_;
    };

template <typename Func, typename Resp>
void ResponseLogger(Func send, Resp resp){
        std::string cont_type;
        resp.count("Content-Type") ? cont_type = resp.at("Content-Type") : cont_type = "null";
        auto start = steady_clock::now();
        send(resp);
        auto finish = steady_clock::now();
        int resp_time = duration_cast<milliseconds>(finish-start).count();
        json::value response_data{{"response_time", resp_time}, {"code", resp.result_int()}, {"content_type", cont_type}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, response_data) << "response sent"sv;
}

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    public:
        using Strand = net::strand<net::io_context::executor_type>;

        explicit RequestHandler(app::Application& app, Strand api_strand)
                :  api_handler_(app), static_handler_(app.GetDocRootPath()), api_strand_(api_strand) {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(net::ip::tcp::endpoint endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
        {
            json::value request_data{{"ip", endpoint.address().to_string()}, {"URI", std::string(req.target())}, {"method", req.method_string()}};
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, request_data) << "request received"sv;

            if(ApiRequestHandler::IsApiRequest(req)){

                auto handle = [self = shared_from_this(), &send, &req ] {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                    assert(self->api_strand_.running_in_this_thread());

                    auto resp = std::move(self->api_handler_.Handle(req));
                    ResponseLogger(send, resp);
                    return;
                };

                return net::dispatch(api_strand_, handle);
            }
            else{
                //STATIC file
                auto variant_resp = static_handler_.Handle(req);
                if(std::holds_alternative<StringResponse>(variant_resp)){

                    auto resp = std::move(get<StringResponse>(variant_resp));
                    ResponseLogger(send, std::move(resp));
                }
                else if(std::holds_alternative<FileResponse>(variant_resp)){

                    auto resp = std::move(get<FileResponse>(variant_resp));
                    ResponseLogger(send, std::move(resp));
                }
                else{

                    auto resp = std::move(get<EmptyBodyResponse>(variant_resp));
                    ResponseLogger(send, std::move(resp));
                }
            }

        }
    private:
        ApiRequestHandler api_handler_;
        StaticRequestHandler static_handler_;
        Strand api_strand_;
    };

}  // namespace http_handler

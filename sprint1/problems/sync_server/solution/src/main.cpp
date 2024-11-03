// Подключаем заголовочный файл <sdkddkver.h> в системе Windows,
// чтобы избежать предупреждения о неизвестной версии Platform SDK,
// когда используем заголовочные файлы библиотеки Boost.Asio
#ifdef WIN32
#include <sdkddkver.h>
#endif
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <thread>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType{
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};
std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer){
    beast::error_code err_code;
    StringRequest request;
    http::read(socket, buffer, request, err_code);

    if(err_code == http::error::end_of_stream){
        return std::nullopt;
    }

    if(err_code){
        throw std::runtime_error("Error while reading request! "s.append(err_code.message()));
    }

    return request;
}

void DumpRequest(StringRequest& request){
    std::cout << request.method_string() << ' ' << request.target() << std::endl;

    for(const auto& header : request){
        std::cout << " "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

StringResponse MakeStringResponse(http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML){
    StringResponse response(http::status::ok, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(response.body().size());
    response.keep_alive(keep_alive);
}

StringResponse MakeStringGetResponse(http::status status, std::string_view body,
                                     unsigned http_version, bool keep_alive,
                                     std::string_view content_type = ContentType::TEXT_HTML){
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.keep_alive(keep_alive);
    response.content_length(body.size());
    return response;
}

StringResponse MakeStringHeadResponse(http::status status, size_t body_size,
                                     unsigned http_version, bool keep_alive,
                                     std::string_view content_type = ContentType::TEXT_HTML){
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.keep_alive(keep_alive);
    response.content_length(body_size);
    return response;
}

StringResponse MakeStringOtherResponse(http::status status, unsigned http_version, bool keep_alive,
                                      std::string_view content_type = ContentType::TEXT_HTML){
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.set(http::field::allow, "GET, HEAD");
    response.keep_alive(keep_alive);
    response.body() = "Invalid method"sv;
    response.content_length(response.body().size());
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    switch (req.method()) {
        case http::verb::get:{
            std::stringstream str;
            str << "Hello, " << req.target().substr(1);
            return MakeStringGetResponse(http::status::ok, str.str(), req.version(), req.keep_alive());
        }
        case http::verb::head:{
            std::stringstream str;
            str << "Hello, " << req.target().substr(1);
            return MakeStringHeadResponse(http::status::ok, str.str().size(), req.version(), req.keep_alive());
        }
        default:
            return MakeStringOtherResponse(http::status::method_not_allowed, req.version(), req.keep_alive());
    }
}

template <typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& request_handler){
    try{
        beast::flat_buffer buffer;

        while(auto request = ReadRequest(socket, buffer)){
            DumpRequest(*request);
            StringResponse response = request_handler(*std::move(request));
            http::write(socket, response);
            if(response.need_eof()){
                break;
            }
        }
    }catch (std::exception& e){
        std::cout << e.what() << std::endl;
    }

    beast::error_code  err_code;
    socket.shutdown(tcp::socket::shutdown_send, err_code);
}
int main(){
    net::io_context io_ctx;
    const auto ip_adress = net::ip::make_address("0.0.0.0");
    constexpr short port = 8080;

    tcp::acceptor acceptor(io_ctx, {ip_adress, port});
    std::cout << "Waiting for socket connection..."sv << std::endl;

     while(true){
         tcp::socket socket(io_ctx);
         acceptor.accept(socket);

         std::thread t([](tcp::socket socket){
             HandleConnection(socket, HandleRequest);
         }, std::move(socket));

         t.detach();
     }
}
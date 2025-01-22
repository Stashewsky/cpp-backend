#pragma once

#include <optional>
#include <mutex>
#include <filesystem>
#include <boost/asio.hpp>

#include "token.h"
#include "tools.h"

#include "response.h"
#include "magic_defs.h"
#include "uri_api.h"

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace sys = boost::system;
    namespace fs = std::filesystem;
    using namespace std::literals;

    class StaticRequestHandler {

    public:
        explicit StaticRequestHandler(const std::string& doc_root)
        : doc_root_(doc_root), uri_handler_(){
        }

        template <typename Body, typename Allocator>
        std::variant<StringResponse, FileResponse, EmptyBodyResponse>
                        Handle(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            if(req.method() != http::verb::get &&
               req.method() != http::verb::head ) {
                return Response::MakeMethodNotAllowed("Invalid method"sv, "GET, HEAD"sv);
            }

            const std::string target = std::string(req.target());

            fs::path base_path = fs::weakly_canonical(fs::absolute(fs::path(doc_root_)));
            fs::path rel_path{tools::DecodeUrl(target).substr(1)};
            fs::path abs_path = fs::weakly_canonical(fs::path(base_path / rel_path));
            if(target.back() == '/'){
                abs_path.append("index.html");
            }

            if(!tools::IsSubPath( abs_path, base_path)){
                return Response::MakeBadRequestText();
            }

            beast::error_code ec;
            http::file_body::value_type body;
            body.open(abs_path.c_str(), beast::file_mode::scan, ec);

            if(ec == beast::errc::no_such_file_or_directory){
               return Response::MakeNotFoundText();
            }

            if(ec){
                return Response::MakeServerErrorText(ec.message());
            }

            auto const size = body.size();

            if(req.method() == http::verb::head)
            {
                http::response<http::empty_body> res{http::status::ok, req.version()};
                res.set(http::field::content_type, tools::GetMimeType(abs_path));
                res.content_length(size);
                res.keep_alive(req.keep_alive());
                return res;
            }

            http::response<http::file_body> res{
                    std::piecewise_construct,
                    std::make_tuple(std::move(body)),
                    std::make_tuple(http::status::ok, req.version())};

            res.set(http::field::content_type, tools::GetMimeType(abs_path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;

        }

    private:
        std::string doc_root_;
        uri_api::UriData uri_handler_;
    };


}

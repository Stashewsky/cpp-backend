#pragma once
#include <filesystem>

#include "http_server.h"
#include "model.h"
#include "api_response_bank.h"
#include "static_content_response_bank.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, fs::path sc_root_path)
            : game_{game},
            static_content_root_{sc_root_path}
    {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (responseBank::CheckMakeBadRequestResponse(req, game_)) {
            send(responseBank::MakeResponseBadRequest(req, game_));
            return;
        }

        if (responseBank::CheckMakeGetMapListResponse(req, game_)) {
            HandleGetMapList(req, send);
            return;
        }

        if (responseBank::CheckMakeMapNotFoundResponse(req, game_)) {
            HandleMapNotFound(req, send);
            return;
        }

        if (responseBank::CheckMakeGetMapByIdResponse(req, game_)) {
            HandleGetMapById(req, send);
            return;
        }

        if(CheckStaticContentFileNotFound(req, static_content_root_)) {       // static content
            HandleFileNotFound(req, send);
            return;
        }

        if(CheckLeaveStaticContentRootDir(req, static_content_root_)) {
            HandleLeaveStaticContentRootDir(req, send);
            return;
        }

        if(CheckGetStaticContentFile(req, static_content_root_)) {
            HandleGetStaticContent(req, send);
        }

        send(responseBank::MakeResponsePageNotFound(req, game_));
    }
private:
    model::Game& game_;
    fs::path static_content_root_;

    // Отдельные методы для обработки конкретных запросов
    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapList(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() == http::verb::get) {
            send(responseBank::MakeResponseGetMapList(req, game_));
        } else {
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleMapNotFound(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() == http::verb::get) {
            send(responseBank::MakeResponseMapNotFound(req, game_));
        } else {
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapById(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() == http::verb::get) {
            send(responseBank::MakeResponseGetMapById(req, game_));
        } else {
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleFileNotFound(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send){
        if(req.method() == http::verb::get || req.method() == http::verb::head){
            send(responseBank::MakeResponseFileNotFound(req, static_content_root_));
        }else{
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleLeaveStaticContentRootDir(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send){
        if(req.method() == http::verb::get || req.method() == http::verb::head){
            send(responseBank::MakeResponseLeaveStaticContentRootDir(req, static_content_root_));
        }else{
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetStaticContent(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send){
        if(req.method() == http::verb::get || req.method() == http::verb::head){
            send(responseBank::MakeResponseGetStaticContent(req, static_content_root_));
        }else{
            send(responseBank::MakeResponseBadRequest(req, game_));
        }
    }
};

}  // namespace http_handler

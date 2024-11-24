#pragma once
#include "http_server.h"
#include "model.h"
#include "response_bank.h"
namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
            : game_{game} {
    }

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

        send(responseBank::MakeResponsePageNotFound(req, game_));
    }

private:
    model::Game& game_;

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
};

}  // namespace http_handler

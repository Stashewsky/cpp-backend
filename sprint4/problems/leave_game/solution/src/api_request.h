#pragma once

#include <optional>
#include <mutex>
#include <boost/asio.hpp>
#include <string>

#include "token.h"
#include "model.h"
#include "application.h"
#include "json_loader.h"

#include "response.h"
#include "magic_defs.h"
#include "uri_api.h"

namespace http_handler
{
    class ApiRequestHandler
    {
    public:
        explicit ApiRequestHandler(app::Application& application)
        : uri_handler_(), application_(application) {

            //Добавляем ендпоинт без авторизации и с разрешенным методами POST
            LinkJoinGameWithoutAuthorize();

            //Добавляем список игроков с GET и авторизацией
            LinkPlayersList();
            LinkGameState();
            LinkPlayerAction();
            LinkTimeTick();
            LinkRecords();

            //Обработчики карт
            LinkMaps();
            LinkMapId();
        }

        template <typename Body, typename Allocator>
        static bool IsApiRequest(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            return req.target().starts_with(Endpoint::API);
        }

        template <typename Body, typename Allocator>
        static bool IsGameRequest(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            return req.target().starts_with(Endpoint::GAME);
        }

        StringResponse ProcessJoinGame(const std::string& target, const std::string& body);
        StringResponse ProcessTimeTick(const std::string& target, const std::string& body);
        StringResponse ProcessRecords(const std::string& target, const std::string& body);
        StringResponse ProcessGetPlayersList(const Token& token, std::string_view body);
        StringResponse ProcessGetGameState(const Token& token, std::string_view body);
        StringResponse ProcessPlayerAction(const Token& token, std::string body);
        StringResponse ProcessGetMaps(const std::string& target, const std::string& body) const;
        StringResponse ProcessGetMapId(const std::string& target, const std::string& body);

        template <typename Body, typename Allocator>
        StringResponse ProcessGameRequest(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            return uri_handler_.Process(req);
        }

        template <typename Body, typename Allocator>
        auto HandleGameRequest(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            auto response = ProcessGameRequest(req);
            response.keep_alive(req.keep_alive());
            response.version(req.version());
            return response;
        }

        template <typename Body, typename Allocator>
        StringResponse Handle(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            return {std::move(HandleGameRequest(req))};
        }

    private:
        uri_api::UriData uri_handler_;
        app::Application& application_;

        void LinkMaps();
        void LinkMapId();
        void LinkJoinGameWithoutAuthorize();
        void LinkPlayersList();
        void LinkGameState();
        void LinkPlayerAction();
        void LinkTimeTick();
        void LinkRecords();

    };
}


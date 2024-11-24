#pragma once
#include "model.h"
#include "json_builder.h"
#include <vector>
#include <boost/beast/http.hpp>

namespace responseBank{

    namespace beast = boost::beast;
    namespace http = beast::http;
    using StringResponse = http::response<http::string_body>;

    std::vector<std::string_view> SplitUrl(std::string_view str);

    template <typename Body, typename Allocator>
    bool CheckMakeGetMapListResponse(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        return req.target() == "/api/v1/maps" or req.target() == "/api/v1/maps/";
    }

    template <typename Body, typename Allocator>
    StringResponse MakeResponseGetMapList(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game) {
        StringResponse response(http::status::ok, req.version());
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(jsonBuilder::BuildMapListJson(game));
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    }

    template <typename Body, typename Allocator>
    bool CheckMakeGetMapByIdResponse(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        auto url = SplitUrl(req.target());
        return url.size()==4 &&
               url[0] == "api" &&
               url[1] == "v1" &&
               url[2] == "maps" &&
               game.FindMap(model::Map::Id(std::string(url[3]))) != nullptr;
    }

    template <typename Body, typename Allocator>
    StringResponse MakeResponseGetMapById(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game) {
        StringResponse response(http::status::ok, req.version());
        auto id = SplitUrl(req.target())[3];
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(jsonBuilder::BuildMapJson(*game.FindMap(model::Map::Id(id))));
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    }


    template <typename Body, typename Allocator>
    bool CheckMakeBadRequestResponse(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        auto url = SplitUrl(req.target());
        return !url.empty() &&
               url[0] == "api" &&
               (
                       url.size() > 4 ||
                       url.size() < 3 ||
                       (url.size() >= 2 && url[1] != "v1") ||
                       (url.size() >= 3 && url[2] != "maps")
               );
    };

    template <typename Body, typename Allocator>
    StringResponse MakeResponseBadRequest(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        StringResponse response(http::status::bad_request, req.version());
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(jsonBuilder::BuildBadRequestJsonResponse());
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    };

    template <typename Body, typename Allocator>
    bool CheckMakeMapNotFoundResponse(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        auto url = SplitUrl(req.target());
        return url.size()==4 &&
               url[0] == "api" &&
               url[1] == "v1" &&
               url[2] == "maps" &&
               game.FindMap(model::Map::Id(std::string(url[3]))) == nullptr;
    };

    template <typename Body, typename Allocator>
    StringResponse MakeResponseMapNotFound(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        StringResponse response(http::status::not_found, req.version());
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(jsonBuilder::BuildMapNotFoundJsonResponse());
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    };

    template <typename Body, typename Allocator>
    StringResponse MakeResponsePageNotFound(
            const http::request<Body, http::basic_fields<Allocator>>& req,
            const model::Game& game){
        StringResponse response(http::status::not_found, req.version());
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(jsonBuilder::BuildPageNotFoundJsonResponse());
        response.content_length(response.body().size());
        response.keep_alive(req.keep_alive());
        return response;
    };

}
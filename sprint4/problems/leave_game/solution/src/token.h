#pragma once

#include <algorithm>
#include <string>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

#include "tagged.h"
#include "response.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

namespace security
{
    static inline std::optional<Token> ExtractTokenFromStringViewAndCheckIt(std::string_view body)
    {
        if(body.size() != 39){
            return std::nullopt;
        }

        std::string token = std::string(body).substr(7);
        return {Token(token)};
    }

    template <typename Fn, typename Body, typename Allocator>
    http_handler::StringResponse ExecuteAuthorized(const boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& req, Fn&& action) 
    {
        if(!req.base().count(boost::beast::http::field::authorization)){
            return http_handler::Response::MakeUnauthorizedErrorInvalidToken();
        }

        if (auto token = ExtractTokenFromStringViewAndCheckIt(req.base()[boost::beast::http::field::authorization])) 
        {
            return action(*token, req.body());
        }
        else 
        {
            return http_handler::Response::MakeUnauthorizedErrorInvalidToken();
        }
    }
}

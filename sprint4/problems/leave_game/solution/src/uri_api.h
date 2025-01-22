#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "magic_defs.h"
#include "response.h"
#include "token.h"

#include <string_view>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <variant>

namespace uri_api
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    using FunctionWithAuthorize = std::function<http_handler::StringResponse(const Token& token, const std::string_view& body)>;
    using FunctionWithoutAuthorize = std::function<http_handler::StringResponse(const std::string_view& body)>;
    using FunctionTargetBody = std::function<http_handler::StringResponse(const std::string& target, const std::string& body)>;

    class UriElement
    {
        struct AllowedMethods
        {
            std::vector<http::verb> data_;
            std::string_view error_;
            std::string_view allowed_;

            AllowedMethods()
            : data_()
            , error_()
            , allowed_()
            {};
        };

        struct AuthorizeData
        {
            bool need_;
            AuthorizeData()
            : need_(false)
            {};
        };

        struct ContentType
        {
            bool need_to_check_;
            std::string_view value_;
            std::string_view error_;

            ContentType()
            : need_to_check_(false)
            {};
        };
    public:
        UriElement()
        : methods_()
        , authorize_()
        , content_type_()
        {};
        UriElement& SetAllowedMethods(std::vector<http::verb> methods, std::string_view method_error_message,
                                      std::string_view allowed_methods)
        {
            methods_.data_ = std::move(methods);
            methods_.error_ = method_error_message;
            methods_.allowed_ = allowed_methods;

            return *this;
        }

        UriElement& SetContentType(std::string_view type, std::string_view error_message)
        {
            content_type_.need_to_check_ = true;
            content_type_.value_ = type;
            content_type_.error_ = error_message;
            return *this;
        }

        UriElement& SetNeedAuthorization(bool need_authorize_)
        {
            authorize_.need_ = need_authorize_;
            return *this;
        }

        UriElement& SetProcessFunction(FunctionWithAuthorize f)
        {
            process_function_ = std::move(f);
            return *this;
        }
        UriElement& SetProcessFunction(FunctionWithoutAuthorize f)
        {
            process_function_without_authorize_ = std::move(f);
            return *this;
        }
        UriElement& SetProcessFunction(FunctionTargetBody f)
        {
            process_function_target_body_ = std::move(f);
            return *this;
        }


        template <typename Body, typename Allocator>
        http_handler::StringResponse ProcessRequest(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            if( methods_.data_.empty() || std::find(methods_.data_.begin(), methods_.data_.end(), req.method()) != methods_.data_.end())
            {
                //Check content type if necessary
                if(content_type_.need_to_check_ && content_type_.value_ != req.base()[boost::beast::http::field::content_type])
                {
                    return http_handler::Response::MakeBadRequestInvalidArgument(content_type_.error_);
                }
                if(authorize_.need_)
                {
                    return security::ExecuteAuthorized(req, [&](const Token& token, std::string_view body){
                        return process_function_(token, body);
                    });
                }

                const std::string target = std::string(req.target());
                const std::string body = std::string(req.body());
                return process_function_target_body_(target, body);
            }
            
            return http_handler::Response::MakeMethodNotAllowed(methods_.error_, methods_.allowed_);
        }
    private:
        AllowedMethods methods_;
        AuthorizeData authorize_;
        ContentType content_type_;
        FunctionWithAuthorize process_function_;
        FunctionWithoutAuthorize process_function_without_authorize_;
        FunctionTargetBody process_function_target_body_;
    };

    class UriData
    {
    public:
        UriData() = default;
        UriElement* AddEndpoint(std::string_view uri)
        {
            auto [it, ret] = data_.try_emplace(std::string(uri));

            return &it->second;
        }

        template <typename Body, typename Allocator>
        http_handler::StringResponse Process(http::request<Body, http::basic_fields<Allocator>>& req)
        {
            auto stop = req.target().find('?');
            std::string target = std::string(req.target().substr(0, stop));

            if((target.find("/api/v1/maps/"s) != std::string::npos)){
                target = "/api/v1/maps/"s;
            }

            auto it = data_.find(target);
            if(it != data_.end())
            {
                return it->second.ProcessRequest(req);
            }
            return http_handler::Response::MakeJSON(http::status::bad_request, ErrorCode::BAD_REQUEST,
                                                    ErrorMessage::INVALID_ENDPOINT);
        }
    private:
        std::unordered_map<std::string, UriElement> data_;
    };
}

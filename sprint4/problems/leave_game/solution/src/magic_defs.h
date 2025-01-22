#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <string_view>
#include <string>
using namespace std::literals;

struct ServerMessage
{
    static inline constexpr std::string_view START = "Server has started..."sv;
    static inline constexpr std::string_view EXIT = "server exited"sv;
};

struct ServerAction
{
    static inline constexpr std::string_view READ = "read"sv;
    static inline constexpr std::string_view WRITE = "write"sv;
    static inline constexpr std::string_view ACCEPT = "accept"sv;
};

struct Width {
    static inline const double ITEM_WIDTH = 0.0;
    static inline const double PLAYER_WIDTH = 0.6;
    static inline const double OFFICE_WIDTH = 0.5;

};

struct JsonField
{
    static inline const std::string CODE = "code"s;
    static inline const std::string MESSAGE = "message"s;
    static inline const std::string USERNAME = "userName"s;
    static inline const std::string MAPID = "mapId"s;

    static inline const std::string ID = "id"s;
    static inline const std::string NAME = "name"s;
    static inline const std::string X = "x"s;
    static inline const std::string Y = "y"s;
    static inline const std::string W = "w"s;
    static inline const std::string H = "h"s;
    static inline const std::string X0 = "x0"s;
    static inline const std::string Y0 = "y0"s;
    static inline const std::string X1 = "x1"s;
    static inline const std::string Y1 = "y1"s;
    static inline const std::string OffsetX = "offsetX"s;
    static inline const std::string OffsetY = "offsetY"s;

};

struct ServerParam
{
    static inline constexpr std::string_view ADDR = "0.0.0.0"sv;
    static inline const uint32_t PORT = 8080;
};

struct ErrorCode
{
    static inline constexpr std::string_view MAP_NOT_FOUND = "mapNotFound"sv;
    static inline constexpr std::string_view BAD_REQUEST = "badRequest"sv;
    static inline constexpr std::string_view INVALID_METHOD = "invalidMethod"sv;
    static inline constexpr std::string_view INVALID_ARGUMENT = "invalidArgument"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "invalidToken"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "unknownToken"sv;
};

struct ErrorMessage
{
    static inline constexpr std::string_view MAP_NOT_FOUND = "Map not found"sv;
    static inline constexpr std::string_view JSON_CONTENT_TYPE = "Required Content-type: application/json"sv;
    static inline constexpr std::string_view FILE_NOT_EXIST = "File doesn't exist"sv;
    static inline constexpr std::string_view BAD_REQUEST = "Bad request"sv;
    static inline constexpr std::string_view INVALID_ENDPOINT = "Invalid endpoint"sv;
    static inline constexpr std::string_view INVALID_NAME = "Invalid name"sv;
    static inline constexpr std::string_view POST_IS_EXPECTED = "Only POST method is expected"sv;
    static inline constexpr std::string_view GET_IS_EXPECTED = "Only GET method is expected"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "Authorization header is missing"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "Player token has not been found"sv;
    static inline constexpr std::string_view PARSE_JSON = "Join game request parse error"sv;
    static inline constexpr std::string_view PARSE_PLAYER_ACTION = "Failed to parse action"sv;
    static inline constexpr std::string_view PARSE_TIMER_TICK = "Failed to parse tick request JSON"sv;
    static inline constexpr std::string_view PARSE_MAXITEMS = "maxItems > 100"sv;

};

struct MiscDefs
{
    static inline constexpr std::string_view NO_CACHE = "no-cache"sv;
};

struct MiscMessage
{
    static inline constexpr std::string_view ALLOWED_POST_METHOD = "POST"sv;
    static inline constexpr std::string_view ALLOWED_GET_HEAD_METHOD = "GET, HEAD"sv;
};

struct Endpoint
{
    static inline constexpr std::string_view API = "/api/"sv;
    static inline constexpr std::string_view GAME = "/api/v1/game/"sv;
    static inline constexpr std::string_view MAPS = "/api/v1/maps"sv;
    static inline constexpr std::string_view MAPSID = "/api/v1/maps/"sv;
    static inline constexpr std::string_view JOIN_GAME = "/api/v1/game/join"sv;
    static inline constexpr std::string_view PLAYERS_LIST = "/api/v1/game/players"sv;
    static inline constexpr std::string_view GAME_STATE = "/api/v1/game/state"sv;
    static inline constexpr std::string_view PLAYER_ACTION = "/api/v1/game/player/action"sv;
    static inline constexpr std::string_view TIME_TICK = "/api/v1/game/tick"sv;
    static inline constexpr std::string_view RECORDS = "/api/v1/game/records"sv;
};

struct Records{
    static inline const int RECORDS_MAXITEMS = 100;
};


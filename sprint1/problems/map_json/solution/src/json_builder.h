#pragma once
#include "model.h"
#include <string>
#include <boost/json.hpp>

namespace json = boost::json;

namespace jsonBuilder{

    json::value BuildMapListJson(const model::Game& game);
    json::value BuildMapJson(const model::Map& map);
    json::value BuildMapNotFoundJsonResponse();
    json::value BuildBadRequestJsonResponse();
    json::value BuildPageNotFoundJsonResponse();
}
#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <boost/json.hpp>

namespace extra{
    namespace json = boost::json;
    namespace sys = boost::system;
    using namespace std::literals;

    struct ExtraData{
        //map_id, json::lootTypes
        std::unordered_map<std::string, json::array> trophis;
    };

}

#include "json_builder.h"
#include <map>
#include <sstream>

namespace jsonBuilder{
    json::value BuildMapListJson(const model::Game& game){
        json::array json_arr;

        for(const auto& item : game.GetMaps()){
            json::object map;
            map["id"] = *item.GetId();
            map["name"] = item.GetName();
            json_arr.emplace_back(std::move(map));
        }
        return json_arr;
    }

    void AddRoadsToJson(const model::Map& map, json::object& root){
        json::array roads;
        for(const auto& item : map.GetRoads()){
            json::object road;
            road["x0"] = item.GetStart().x;
            road["y0"] = item.GetStart().y;
            if(item.IsVertical()){
                road["y1"] = item.GetEnd().y;
            }else {
                road["x1"] = item.GetEnd().x;
            }
            roads.emplace_back(std::move(road));
        }
        root["roads"] = std::move(roads);
    }

    void AddBuildingsToJson(const model::Map& map, json::object& root){
        json::array buildings;
        for(const auto& item : map.GetBuildings()){
            json::object building;
            building["x"] = item.GetBounds().position.x;
            building["y"] = item.GetBounds().position.y;
            building["w"] = item.GetBounds().size.width;
            building["h"] = item.GetBounds().size.height;
            buildings.emplace_back(std::move(building));
        }
        root["buildings"] = std::move(buildings);
    }

    void AddOfficesToJson(const model::Map& map, json::object& root){
        json::array offices;
        for(const auto& item : map.GetOffices()){
            json::object office;
            office["id"] = (*item.GetId()).c_str();
            office["x"] = item.GetPosition().x;
            office["y"] = item.GetPosition().y;
            office["offsetX"] = item.GetOffset().dx;
            office["offsetY"] = item.GetOffset().dy;
            offices.emplace_back(std::move(office));
        }
        root["offices"] = std::move(offices);
    }

    json::value BuildMapJson(const model::Map& map){
        json::object map_json;
        map_json["id"] = std::string(*map.GetId());
        map_json["name"] = map.GetName();
        AddRoadsToJson(map, map_json);
        AddBuildingsToJson(map, map_json);
        AddOfficesToJson(map, map_json);
        return map_json;
    }

    json::value BuildMapNotFoundJsonResponse(){
        json::object root;
        root["code"] = "mapNotFound";
        root["message"] = "Map not found";
        return root;
    };

    json::value BuildBadRequestJsonResponse(){
        json::object root;
        root["code"] = "badRequest";
        root["message"] = "Bad request";
        return root;
    };

    json::value BuildPageNotFoundJsonResponse(){
        json::object root;
        root["code"] = "pageNotFound";
        root["message"] = "Page not found";
        return root;
    };
}
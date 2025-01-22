#include "json_loader.h"
#include <boost/json.hpp>

#include <fstream>
#include <memory>
#include <vector>
#include <utility>
#include <stdexcept>
#include "magic_defs.h"

namespace json_loader {

    namespace json = boost::json;
    namespace sys = boost::system;
    using namespace std::literals;

    std::string ReadFile(const std::string& file_name) {
        std::ifstream f(file_name);
        if(!f.is_open()){
            throw std::runtime_error("Error: can not open file: "s + file_name);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    void LoadRoads(std::shared_ptr<model::Map>& model_map, const json::object& obj_map){
        auto const& list_roads = obj_map.at("roads").as_array();
        for(const auto& road : list_roads){
            auto const& obj_road = road.as_object();
            int x0 = obj_road.at(JsonField::X0).as_int64();
            int y0 = obj_road.at(JsonField::Y0).as_int64();
            int end1 = 0;
            std::shared_ptr<model::Road> model_road;
            if(obj_road.contains(JsonField::X1)){
                end1 = obj_road.at(JsonField::X1).as_int64();
                model_road = std::make_shared<model::Road>(model::Road::HORIZONTAL, model::Point{x0, y0}, end1);
            }
            if(obj_road.contains(JsonField::Y1)){
                end1 = obj_road.at(JsonField::Y1).as_int64();
                model_road = std::make_shared<model::Road>(model::Road::VERTICAL, model::Point{x0, y0}, end1);
            }
            model_map->AddRoad(*model_road);
        }
    }

    void LoadBuildings(std::shared_ptr<model::Map>& model_map, const json::object& obj_map){
        auto const& list_buildings = obj_map.at("buildings").as_array();
        for(const auto& building : list_buildings){
            auto const& obj_building = building.as_object();
            int x = obj_building.at(JsonField::X).as_int64();
            int y = obj_building.at(JsonField::Y).as_int64();
            int w = obj_building.at(JsonField::W).as_int64();
            int h = obj_building.at(JsonField::H).as_int64();

            std::shared_ptr<model::Building> model_building =
                    std::make_shared<model::Building>(model::Rectangle{model::Point{x, y}, model::Size{w, h}});
            model_map->AddBuilding(*model_building);
        }
    }

    void LoadOffices(std::shared_ptr<model::Map>& model_map, const json::object& obj_map){
        auto const& list_offices = obj_map.at("offices").as_array();
        for(const auto& office : list_offices){
            auto const& obj_office = office.as_object();
            std::string office_id = static_cast<std::string>(obj_office.at(JsonField::ID).as_string());
            int x = obj_office.at(JsonField::X).as_int64();
            int y = obj_office.at(JsonField::Y).as_int64();
            int offset_x = obj_office.at(JsonField::OffsetX).as_int64();
            int offset_y = obj_office.at(JsonField::OffsetY).as_int64();

            std::shared_ptr<model::Office> model_office =
                    std::make_shared<model::Office>(model::Office::Id{office_id}, model::Point{x, y},
                                                    model::Offset{offset_x, offset_y});
            model_map->AddOffice(*model_office);
        }
    }

    void LoadMaps(model::Game& game, extra::ExtraData& extra_data, const json::value& jv){

        if(jv.get_object().contains("defaultDogSpeed")){
            double game_speed = jv.get_object().at("defaultDogSpeed").as_double();
            game.SetDefaultSpeed(game_speed);
        }
        if(jv.get_object().contains("defaultBagCapacity")){
            int game_bag_capacity = jv.get_object().at("defaultBagCapacity").as_int64();
            game.SetDefaultBagCapacity(game_bag_capacity);
        }
        if(jv.get_object().contains("dogRetirementTime")){
            double dog_retirement_time_ms = jv.get_object().at("dogRetirementTime").as_double();    //sec.
            dog_retirement_time_ms *= 1000; //ms.
            game.SetDefaultRetirementTime(dog_retirement_time_ms);
        }
        if(jv.get_object().contains("lootGeneratorConfig")){
            auto const& obj_loot_genrator = jv.get_object().at("lootGeneratorConfig").as_object();
            double period = obj_loot_genrator.at("period").as_double();
            double probability = obj_loot_genrator.at("probability").as_double();
            game.SetLootGeneratorConfig(period, probability);
        }
        auto const& obj_head = jv.get_object().at("maps");
        auto const& list_maps = obj_head.as_array();
        for (const auto& map : list_maps) {
            auto const& obj_map = map.as_object();
            std::string map_id = static_cast<std::string>(obj_map.at(JsonField::ID).as_string());
            std::string map_name = static_cast<std::string>(obj_map.at(JsonField::NAME).as_string());
            std::shared_ptr<model::Map> model_map = std::make_shared<model::Map>(model::Map::Id{map_id}, map_name);
            //dogSpeed
            if(obj_map.contains("dogSpeed")){
                double map_speed = obj_map.at("dogSpeed").as_double();
                model_map->SetDefaultSpeed(map_speed);
            }
            else
            {
                double map_speed = game.GetDefaultSpeed();
                model_map->SetDefaultSpeed(map_speed);
            }
            //bagCapacity
            if(obj_map.contains("bagCapacity")){
                int bag_capacity = obj_map.at("bagCapacity").as_int64();
                model_map->SetBagCapacity(bag_capacity);
            }
            else
            {
                int bag_capacity = game.GetDefaultBagCapacity();
                model_map->SetBagCapacity(bag_capacity);
            }
            auto const& list_loot_types = obj_map.at("lootTypes").as_array();

            for(const auto& loot_type : list_loot_types){
                auto const& obj_loot_type = loot_type.get_object();
                if(obj_loot_type.contains("value")){
                    int value = obj_loot_type.at("value").as_int64();
                    model_map->AddScoreValue(value);
                }
            }
            extra_data.trophis.emplace(map_id, list_loot_types);
            size_t count_trophy = list_loot_types.size();
            model_map->SetCountLootTypes(count_trophy);

            LoadRoads(model_map, obj_map);

            LoadBuildings(model_map, obj_map);

            LoadOffices(model_map, obj_map);

            game.AddMap(*model_map);
        }
    }


    std::pair<model::Game, extra::ExtraData> LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла

        model::Game game;
        extra::ExtraData extra_data;

        auto jv = json::parse(ReadFile(json_path.string()));

        LoadMaps(game, extra_data, jv);

        return std::pair{game, extra_data};
    }

    std::string GetGameMaps(const model::Game &game) {
        const std::vector<model::Map>& maps = game.GetMaps();
        json::array jarray;
        for(const auto& map : maps){
            jarray.emplace_back(json::object{{JsonField::ID, *map.GetId()}, {JsonField::NAME, map.GetName()}});
        }
        std::string str = json::serialize(jarray);
        return str;
    }

    void GetJsonRoads(json::object& jobject, const model::Map& map){
        const auto& roads = map.GetRoads();
        json::array jarray_roads;
        for(const auto& road : roads){
            json::object jobject_road;
            jobject_road.emplace(JsonField::X0, road.GetStart().x);
            jobject_road.emplace(JsonField::Y0, road.GetStart().y);
            road.IsHorizontal() ? jobject_road.emplace(JsonField::X1, road.GetEnd().x)
                                : jobject_road.emplace(JsonField::Y1, road.GetEnd().y);

            jarray_roads.emplace_back(jobject_road);
        }
        jobject.emplace("roads", jarray_roads);
    }

    void GetJsonBuildings(json::object& jobject, const model::Map& map){
        const auto& buildings = map.GetBuildings();
        json::array jarray_buildings;
        for(const auto& building : buildings){
            json::object jobject_building;
            jobject_building.emplace(JsonField::X, building.GetBounds().position.x);
            jobject_building.emplace(JsonField::Y, building.GetBounds().position.y);
            jobject_building.emplace(JsonField::W, building.GetBounds().size.width);
            jobject_building.emplace(JsonField::H, building.GetBounds().size.height);

            jarray_buildings.emplace_back(jobject_building);
        }
        jobject.emplace("buildings", jarray_buildings);
    }

    void GetJsonOffices(json::object& jobject, const model::Map& map){
        const auto& offices = map.GetOffices();
        json::array jarray_offices;
        for(const auto& office : offices){
            json::object jobject_office;
            jobject_office.emplace("id", *office.GetId());
            jobject_office.emplace(JsonField::X, office.GetPosition().x);
            jobject_office.emplace(JsonField::Y, office.GetPosition().y);
            jobject_office.emplace(JsonField::OffsetX, office.GetOffset().dx);
            jobject_office.emplace(JsonField::OffsetY, office.GetOffset().dy);

            jarray_offices.emplace_back(jobject_office);
        }
        jobject.emplace("offices", jarray_offices);
    }

    std::string GetGameMapFromId(const model::Game &game, const extra::ExtraData& extra_data, const model::Map::Id& id) {
        auto map = game.FindMap(id);
        json::object jobject;

        jobject.emplace(JsonField::ID, *map->GetId());
        jobject.emplace(JsonField::NAME, map->GetName());
        const auto& loot_types = extra_data.trophis.at(*map->GetId());
        jobject.emplace("lootTypes", loot_types);

        GetJsonRoads(jobject, *map);
        GetJsonBuildings(jobject, *map);
        GetJsonOffices(jobject, *map);

        std::string str = json::serialize(jobject);
        return str;
    }

}  // namespace json_loader

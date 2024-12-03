#include "json_loader.h"
#include "logger.h"
#include <fstream>
#include <iostream>
#include <boost/json.hpp>

namespace json_loader {
    using namespace std::literals;
    namespace json = boost::json;

    namespace coord_constants{
        const std::string X_KEY = "x";
        const std::string X0_KEY = "x0";
        const std::string X1_KEY = "x1";
        const std::string OFFSET_X_KEY = "offsetX";

        const std::string Y_KEY = "y";
        const std::string Y0_KEY = "y0";
        const std::string Y1_KEY = "y1";
        const std::string OFFSET_Y_KEY = "offsetY";

        const std::string W_KEY = "w";
        const std::string H_KEY = "h";
    }

    std::string ReadJsonFile(const std::filesystem::path& json_path){
        std::ifstream file(json_path);
        if(!file.is_open()){
            BOOST_LOG_TRIVIAL(error) << logware::CreateLogMessage("error"sv,
                                                                  logware::ExceptionLogData(EXIT_FAILURE,
                                                                                            "Error: Can't open file. Check this path: "sv,
                                                                                            json_path.string()));
            throw std::runtime_error("Error! Can not open json file!");
        }
        std::string line;
        std::string text;
        while(getline(file, line)){
            text += line;
        }
        file.close();
        return text;
    }

    void MapAddRoads(model::Map& map, const json::value& map_item){
        if(!map_item.as_object().contains("roads")) return;
        for(auto item : map_item.as_object().at("roads").as_array()){
            model::Coord x{static_cast<int>(item.as_object().at(coord_constants::X0_KEY).as_int64())};
            model::Coord y{static_cast<int>(item.as_object().at(coord_constants::Y0_KEY).as_int64())};
            model::Point startPoint(x, y);
            if(item.as_object().contains(coord_constants::X1_KEY)){
                model::Coord end{static_cast<int>(item.as_object().at(coord_constants::X1_KEY).as_int64())};
                map.AddRoad(model::Road(model::Road::HORIZONTAL, startPoint, end));
            }else{
                model::Coord end{static_cast<int>(item.as_object().at(coord_constants::Y1_KEY).as_int64())};
                map.AddRoad(model::Road(model::Road::VERTICAL, startPoint, end));
            }
        }
    }

    void MapAddBuilding(model::Map& map, const json::value& map_item){
        if(!map_item.as_object().contains("buildings")) return;
        for(auto item : map_item.as_object().at("buildings").as_array()){
            model::Coord x{static_cast<int>(item.as_object().at(coord_constants::X_KEY).as_int64())};
            model::Coord y{static_cast<int>(item.as_object().at(coord_constants::Y_KEY).as_int64())};
            model::Coord w{static_cast<int>(item.as_object().at(coord_constants::W_KEY).as_int64())};
            model::Coord h{static_cast<int>(item.as_object().at(coord_constants::H_KEY).as_int64())};
            model::Rectangle rectangle{model::Point{x,y}, model::Size{w, h}};
            map.AddBuilding(model::Building(std::move(rectangle)));
        }
    }
    void MapAddOffices(model::Map& map, const json::value& map_item){
        if(!map_item.as_object().contains("offices")) return;
        for(auto item : map_item.as_object().at("offices").as_array()){
            model::Office::Id office_id(std::string(item.as_object().at("id").as_string()));
            model::Coord x{static_cast<int>(item.as_object().at(coord_constants::X_KEY).as_int64())};
            model::Coord y{static_cast<int>(item.as_object().at(coord_constants::Y_KEY).as_int64())};
            model::Coord dx{static_cast<int>(item.as_object().at(coord_constants::OFFSET_X_KEY).as_int64())};
            model::Coord dy{static_cast<int>(item.as_object().at(coord_constants::OFFSET_Y_KEY).as_int64())};
            map.AddOffice(model::Office(office_id, model::Point(x, y), model::Offset(dx, dy)));
        }
    }

    model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    std::string file{ReadJsonFile(json_path)};
    const auto json_map = json::parse(file);
    model::Game game;

    for(auto item : json_map.as_object().at("maps").as_array()){
        std::string map_id (item.as_object().at("id").as_string());
        std::string map_name (item.as_object().at("name").as_string());
        model::Map map(model::Map::Id(map_id), map_name);
        MapAddRoads(map, item);
        MapAddBuilding(map, item);
        MapAddOffices(map, item);
        game.AddMap(std::move(map));
    }

    return game;
}

}  // namespace json_loader

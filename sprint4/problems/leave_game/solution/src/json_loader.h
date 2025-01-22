#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>

#include "model.h"
#include "extra_data.h"

namespace json_loader {

    std::pair<model::Game, extra::ExtraData> LoadGame(const std::filesystem::path& json_path);

    std::string GetGameMaps(const model::Game& game);
    std::string GetGameMapFromId(const model::Game& game, const extra::ExtraData& extra_data, const model::Map::Id& id);

}  // namespace json_loader

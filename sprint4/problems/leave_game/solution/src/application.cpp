#include "application.h"

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

namespace app{

    //player_id == dog_id;
    Player &Players::Add(size_t dog_id, std::shared_ptr<model::GameSession> session) {

        players_.emplace(dog_id, Player{dog_id, session});

        return players_.at(dog_id);
    }

    std::optional<Player> Players::FindByDogIdAndMapId(size_t dog_id, const model::Map::Id &map_id) {
        Player& player = players_.at(dog_id);
        auto map = player.GetGameSession()->GetMap();
        if(map_id == map->GetId()){
            return {player};
        }
        return std::nullopt;
    }

    size_t Players::GetNextPlayerId() {
        return player_incr_id_++;
    }

    void Players::RestorePlayerIdIncr() {
        player_incr_id_ = players_.size();
    }

    void Players::RemovePlayerById(size_t player_id) {
        if(players_.contains(player_id)){
            players_.erase(player_id);
        }
    }

    std::shared_ptr<app::Player> PlayerTokens::FindPlayerByToken(const Token &token) {
        if(token_to_player_.count(token)){
            return token_to_player_.at(token);
        }
        return nullptr;
    }

    Token PlayerTokens::AddPlayer(const Player &player) {
        Token token{GenerateToken()};
        token_to_player_[token] = std::make_shared<app::Player>(player);
        return token;
    }

    std::string PlayerTokens::GenerateToken() {
        std::stringstream token;
        token << std::hex << std::setfill('0') << std::setw(16) << generator1_() << std::setw(16)  << generator2_();
        return token.str();
    }

    void PlayerTokens::RestorePlayerWithToken(const std::string &token, const Player &player) {
        token_to_player_[Token(token)] = std::make_shared<app::Player>(player);
    }

    void PlayerTokens::RemovePlayer(const Token &token) {
        if(token_to_player_.contains(token)){
            token_to_player_.erase(token);
        }
    }

    void PlayerTokens::RemovePlayer(size_t dog_id) {
        std::string remove_token;
        for(const auto& [token, player] : token_to_player_){
            size_t id = player->GetId();
            if(id == dog_id){
                remove_token = *token;
                break;
            }
        }
        if(!remove_token.empty()){
            RemovePlayer(Token(remove_token));
        }
    }

    std::unordered_map<Token, std::shared_ptr<app::Player>, TokenHasher> PlayerTokens::GetTokenPlayers() const {
        return token_to_player_;
    }

    void Application::LoadGame(const std::filesystem::path &config_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto[game, extra_data] = json_loader::LoadGame(config_path);
        game_ = game;
        extra_data_ = extra_data;
        game_.ComputeRoadRTree();
    }

    model::Game Application::GetGameModel() const {
        return game_;
    }

    extra::ExtraData Application::GetExtraData() const {
        return extra_data_;
    }

    std::pair<Token, std::size_t> Application::JoinGame(const std::string &user_name, const std::string &map_id) {

        auto map = game_.FindMap(model::Map::Id{map_id});
        std::pair<int, int> spawn_point;
        if(args_.randomize_spawn_points){
            spawn_point = map->GetRandomCoordOnRoads();
        } else {
            spawn_point = std::make_pair(map->GetRoads().front().GetStart().x, map->GetRoads().front().GetStart().y);
        }
        
        double default_speed = map->GetDefaultSpeed();
        size_t max_bag_size = map->GetBagCapacity();

        if(auto session = game_.FindSession(model::Map::Id{map_id})) {
            size_t dog_id = players_.GetNextPlayerId();
            auto& player = players_.Add(session->AddDog(dog_id, spawn_point, default_speed,
                                                        user_name, max_bag_size), session);
            auto token = player_tokens_.AddPlayer(player);

            return {token, player.GetId()};
        }

        auto session = game_.AddNewSession(model::Map::Id{map_id});
        size_t dog_id = players_.GetNextPlayerId();
        auto& player = players_.Add(session->AddDog(dog_id, spawn_point, default_speed,
                                                   user_name, max_bag_size), session);
        auto token = player_tokens_.AddPlayer(player);

        return {token, player.GetId()};
    }

    std::vector<std::pair<size_t, std::string>> Application::GetPlayersList(const Token &token) {
        auto player = FindPlayerByToken(token);
        auto session = player->GetGameSession();
        auto id_and_names = session->GetIdAndNames();

        return id_and_names;
    }

    std::shared_ptr<Player> Application::FindPlayerByToken(const Token &token) {
        return player_tokens_.FindPlayerByToken(token);
    }

    std::vector<model::Map> Application::ListMaps() const {
        return game_.GetMaps();
    }

    std::vector<std::shared_ptr<model::GameSession>> Application::GetGameSessions() {
        return game_.GetGameSessions();
    }

    const model::Map *Application::FindMap(const model::Map::Id &id) const {
        return game_.FindMap(id);
    }

    std::string Application::GetDocRootPath() const {
        return args_.www_root;
    }

    size_t Application::GetTickPeriod() const {
        return args_.tick_period;
    }

    void Application::Tick(std::chrono::milliseconds delta) {
        size_t time_ms = delta.count();
        Tick(time_ms);
    }

    void Application::Tick(size_t time_ms) {
        const auto retirement_time = static_cast<size_t>(game_.GetDefaultRetirementTime());
        auto sessions = GetGameSessions();
        for(auto& session : sessions) {
            std::vector<size_t> remove_dog_by_id;

            session->GenerateLoot(static_cast<std::chrono::milliseconds>(time_ms));
            std::vector<collision_detector::Item> items = session->GetLootToItems();
            std::vector<collision_detector::Gatherer> gatherers;
            gatherers.reserve(session->GetCountDogs());

            auto map = session->GetMap();
            for(auto& [id, dog] : session->GetAllDogs()) {
                size_t life_time_ms = 0;
                size_t stop_time_ms = 0;

                auto current_coord = dog.GetCoord();
                auto current_speed = dog.GetSpeed();
                model::CoordD current_point = {current_coord.first, current_coord.second};
                model::CoordD move_point;
                move_point.x = current_coord.first + current_speed.first * time_ms * 0.001;
                move_point.y = current_coord.second + current_speed.second * time_ms * 0.001;

                life_time_ms = dog.AddLifeTime(time_ms);
                if(current_point == move_point){
                    stop_time_ms = dog.AddStopTime(time_ms);
                }
                else
                {
                    auto move_result = map->GetSuccessRoadMoveSimple(current_point, move_point);
                    if(move_result.has_value()){
                        if(move_result->first){
                            dog.SetMoveCoord(move_result->second);
                            gatherers.push_back({id, {current_point.x, current_point.y},
                                                 {move_point.x, move_point.y},
                                                 Width::PLAYER_WIDTH/2});
                            dog.ResetStopTime();
                        }
                        else{
                            dog.SetMoveCoord(move_result->second);
                            dog.SetMoveSpeed(std::pair{0.0, 0.0});
                            gatherers.push_back({id, {current_point.x, current_point.y},
                                                 {move_result->second.x, move_result->second.y},
                                                 Width::PLAYER_WIDTH/2});
                            //stop_time_ms = dog.AddStopTime(time_ms);
                            dog.ResetStopTime();
                        }
                    }
                }

                if(stop_time_ms >= retirement_time){
                    RecordsPlayer record {RecordsPlayerId::New(), dog.GetName(),
                    dog.GetScore(), static_cast<int>(life_time_ms)};
                    db_.GetRecordsPlayer().Save(record);
                    remove_dog_by_id.push_back(id);

                }


            }

            using namespace collision_detector;
            VectorItemGathererProvider provider{ items, gatherers };
            std::vector<GatheringEvent> events = FindGatherEvents(provider);
            for(const auto& event : events) {
                auto& dog = session->GetDogById(event.gatherer_id);
                if(event.item_id >= 9999){
                    std::vector<int> types = dog.ClearBag();
                    for(auto type : types){
                        int score = map->GetScoreByIdx(type);
                        dog.AddScore(score);
                    }
                }
                else
                {
                    if(session->ContainLootById(event.item_id)){
                        int type = session->GetLootTypeById(event.item_id);
                        if(dog.SetBagItem({event.item_id, type})){
                            session->RemoveLootById(event.item_id);
                        }
                    }
                }
            }

            for(const auto& remove_id : remove_dog_by_id){
                session->RemoveDogById(remove_id);
                player_tokens_.RemovePlayer(remove_id);
                players_.RemovePlayerById(remove_id);
            }
        }

        server_time_ms_ += time_ms;
        if(backup_time_ms_ > 0 && server_time_ms_ >= backup_time_ms_){
            backup_time_ms_ = server_time_ms_ + args_.save_state_period;
            BackUpData();
        }
    }

    void Application::BackUpData() {
        if(args_.state_file.empty()){
            return;
        }

        std::vector<SaveSessionData> back_up_data;
        auto sessions = GetGameSessions();
        for(auto& session : sessions) {
            SaveSessionData session_data;
            auto map = session->GetMap();
            session_data.map_id = *map->GetId();        //map_id
            for(auto& [dog_id, dog] : session->GetAllDogs()){
                session_data.dogs_repr.emplace_back(dog_id, dog);
                session_data.loots = session->GetLoot();
            }

            for(const auto& [token, player] : player_tokens_.GetTokenPlayers()){
                session_data.dog_id_to_token[player->GetId()] = *token;
            }
            back_up_data.push_back(session_data);
        }

        std::string temp_file_name = "state_temp.txt";
        fs::path file_path = fs::weakly_canonical(fs::absolute(fs::path(args_.state_file)));
        fs::path temp_file_path = file_path.parent_path() / temp_file_name;
        std::ofstream out(temp_file_path);

        if (out.is_open())
        {
            OutputArchive output_archive{out};
            output_archive << back_up_data;
        }
        out.close();
        fs::rename(temp_file_path, file_path);

    }

    void Application::RestoreBackUpData() {

        if(args_.state_file.empty()){
            return;
        }
        fs::path file_path = fs::weakly_canonical(fs::absolute(fs::path(args_.state_file)));
        if(!fs::exists(file_path)){
            return;
        }

        std::vector<SaveSessionData> back_up_data;
        std::ifstream in(file_path); // окрываем файл для чтения
        if (!in.is_open())
        {
            throw std::logic_error("BackUp file not open");
        }
        InputArchive input_archive{in};
        input_archive >> back_up_data;
        in.close();

        for(const auto& session_data : back_up_data){
            auto map_id = session_data.map_id;

            auto dog_id_to_token = session_data.dog_id_to_token;
            for(const auto& dog_repr : session_data.dogs_repr){
                const auto& [dog_id, dog] = dog_repr.Restore();

                if(auto session = game_.FindSession(model::Map::Id{map_id})) {

                    auto& player = players_.Add(session->AddDog(dog_id, dog), session);

                    player_tokens_.RestorePlayerWithToken(dog_id_to_token[dog_id], player);

                    continue;
                }

                auto session = game_.AddNewSession(model::Map::Id{map_id});
                auto& player = players_.Add(session->AddDog(dog_id, dog), session);

                player_tokens_.RestorePlayerWithToken(dog_id_to_token[dog_id], player);

                session->RestoreLoot(session_data.loots);

            }

        }
        players_.RestorePlayerIdIncr(); //глобальная нумерация игроков

    }

    std::vector<RecordsPlayer> Application::ReadRecords() {
        return db_.GetRecordsPlayer().Read();
    }

    std::vector<RecordsPlayer> Application::ReadRecords(int start, int maxItems) {
        return db_.GetRecordsPlayer().Read(start, maxItems);
    }


}

























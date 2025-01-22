#pragma once

#include "model.h"
#include "token.h"
#include "json_loader.h"
#include "program_options.h"
#include "extra_data.h"
#include "collision_detector.h"
#include <boost/json.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>

#include "model_serialization.h"
#include "postgres.h"

namespace app{
    namespace fs = std::filesystem;
    using namespace std::literals;

    class Player{
    public:
        explicit Player(size_t dog_id, std::shared_ptr<model::GameSession> session) : session_(session), dog_id_(dog_id) {
        }

        [[nodiscard]] size_t GetId() const {
            return dog_id_;
        }

        [[nodiscard]] std::shared_ptr<model::GameSession> GetGameSession() const {
            return session_;
        }

    private:
        std::shared_ptr<model::GameSession> session_ = nullptr;
        size_t dog_id_ = 0;
    };


    class Players{
    public:
        Players() = default;
        Player& Add(size_t dog_id, std::shared_ptr<model::GameSession> session );
        [[nodiscard]] size_t GetNextPlayerId();
        std::optional<Player> FindByDogIdAndMapId(size_t dog_id, const model::Map::Id& map_id);
        void RestorePlayerIdIncr();
        void RemovePlayerById(size_t player_id);
    private:
        size_t player_incr_id_ = 0;
        std::unordered_map<size_t, Player> players_;
    };

    class PlayerTokens {
    public:
        PlayerTokens() = default;
        std::shared_ptr<app::Player> FindPlayerByToken(const Token& token);
        Token AddPlayer(const app::Player& player);
        void RestorePlayerWithToken(const std::string& token, const app::Player& player);
        void RemovePlayer(const Token& token);
        void RemovePlayer(size_t dog_id);
        std::unordered_map<Token, std::shared_ptr<app::Player>, TokenHasher> GetTokenPlayers() const;

    private:
        std::string GenerateToken();
        std::random_device random_device_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};

        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        // Чтобы сгенерировать токен, получите из generator1_ и generator2_
        // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
        // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
        // чтобы сделать их подбор ещё более затруднительным

        std::unordered_map<Token, std::shared_ptr<app::Player>, TokenHasher> token_to_player_;
    };

    struct SaveSessionData{
        //std::unordered_map<size_t, model::Dog> dogs;
        std::vector<serialization::DogRepr> dogs_repr;
        std::unordered_map<size_t, model::Loot> loots;
        std::unordered_map<size_t, std::string> dog_id_to_token;
        std::string map_id;     //for create new session

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& dogs_repr;
            ar& loots;
            ar& dog_id_to_token;
            ar& map_id;
        }
    };

    struct AppConfig {
        std::string db_url;
    };

    class Application{
    public:
        explicit Application(const Args& args, const AppConfig& config): args_(args)
        , db_(pqxx::connection{config.db_url})
        {
            LoadGame(args_.config_file);
        }

        void LoadGame(const std::filesystem::path& config_path);

        model::Game GetGameModel() const;
        extra::ExtraData GetExtraData() const;

        std::pair<Token, std::size_t> JoinGame(const std::string& user_name, const std::string& map_id);

        std::vector<std::pair<size_t, std::string>> GetPlayersList(const Token& token);

        std::shared_ptr<Player> FindPlayerByToken(const Token& token);

        std::vector<model::Map> ListMaps() const;

        std::vector<std::shared_ptr<model::GameSession>> GetGameSessions();

        const model::Map* FindMap(const model::Map::Id& id) const;

        std::string GetDocRootPath() const;

        size_t GetTickPeriod() const;

        void Tick(std::chrono::milliseconds delta);

        void Tick(size_t time_ms);

        void BackUpData();
        void RestoreBackUpData();

        std::vector<RecordsPlayer> ReadRecords();
        std::vector<RecordsPlayer> ReadRecords(int start, int maxItems );

    private:
        model::Game game_;
        extra::ExtraData extra_data_;
        Players players_;
        PlayerTokens player_tokens_;
        Args args_;
        size_t backup_time_ms_ = args_.save_state_period;
        size_t server_time_ms_ = 0;
        postgres::Database db_;
        std::mutex mutex_;
    };

}

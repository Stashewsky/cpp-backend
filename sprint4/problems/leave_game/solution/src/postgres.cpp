#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

    using namespace std::literals;
    using pqxx::operator"" _zv;

    void RecordsPlayerRepositoryImpl::Save(const app::RecordsPlayer &record) {
        pqxx::work work{connection_};
        work.exec_params(
                R"(INSERT INTO retired_players (id, name, score, play_time_ms)
                     VALUES ($1, $2, $3, $4);)"_zv,
                record.id.ToString(), record.name, record.score, record.play_time_ms);
        work.commit();
    }

    std::vector<app::RecordsPlayer> RecordsPlayerRepositoryImpl::Read() {
        std::vector<app::RecordsPlayer> records_player;
        pqxx::read_transaction r(connection_);
        auto query_text = "SELECT id, name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name LIMIT 100;"_zv;

        for (auto [id, name, score, play_time_ms] : r.query<std::string, std::string, int, int>(query_text)) {
            records_player.emplace_back(app::RecordsPlayerId::FromString(id), name, score, play_time_ms);
        }
        r.commit();
        return records_player;
    }

    std::vector<app::RecordsPlayer> RecordsPlayerRepositoryImpl::Read(int start, int maxItems) {
        std::vector<app::RecordsPlayer> records_player;
        pqxx::read_transaction r(connection_);
        std::string query_text = "SELECT id, name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name LIMIT " +
                std::to_string(maxItems) + " OFFSET " + std::to_string(start) + ";";

        for (auto [id, name, score, play_time_ms] : r.query<std::string, std::string, int, int>(query_text)) {
            records_player.emplace_back(app::RecordsPlayerId::FromString(id), name, score, play_time_ms);
        }
        r.commit();
        return records_player;
    }

    Database::Database(pqxx::connection connection) : connection_{std::move(connection)} {
        pqxx::work work{connection_};

        work.exec(R"(CREATE TABLE IF NOT EXISTS retired_players (id UUID PRIMARY KEY,
        name varchar(100) NOT NULL, score integer NOT NULL, play_time_ms integer NOT NULL);)"_zv);

        work.exec(R"(CREATE INDEX IF NOT EXISTS score_play_time_name_idx ON retired_players (score, play_time_ms, name);)"_zv);
        work.commit();
    }


}  // namespace postgres
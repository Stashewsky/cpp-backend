#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <string>
#include <vector>

#include "records_player.h"

namespace postgres {

    class RecordsPlayerRepositoryImpl : public app::RecordsPlayerRepository{
    public:
        explicit RecordsPlayerRepositoryImpl(pqxx::connection& connection)
            : connection_{connection} {
        }
        void Save(const app::RecordsPlayer& record) override;
        std::vector<app::RecordsPlayer> Read() override;
        std::vector<app::RecordsPlayer> Read(int start, int maxItems ) override;

    private:
        pqxx::connection& connection_;
    };


    class Database {
    public:
        explicit Database(pqxx::connection connection);

        RecordsPlayerRepositoryImpl& GetRecordsPlayer() & {
            return records_player_;
        }

    private:
        pqxx::connection connection_;
        RecordsPlayerRepositoryImpl records_player_{connection_};
    };

}  // namespace postgres
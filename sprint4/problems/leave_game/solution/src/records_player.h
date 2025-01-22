#pragma once

#include <string>
#include <vector>
#include "tagged_uuid.h"

namespace app{
    namespace detail {
        struct RecordsPlayerTag {};
    }  // namespace detail

    using RecordsPlayerId = util::TaggedUUID<detail::RecordsPlayerTag>;

    struct RecordsPlayer{
        RecordsPlayerId id;
        std::string name;
        int score = 0;
        int play_time_ms = 0;
    };

    class RecordsPlayerRepository {
    public:
        virtual void Save(const RecordsPlayer& record) = 0;
        virtual std::vector<RecordsPlayer> Read() = 0;
        virtual std::vector<RecordsPlayer> Read(int start, int maxItems ) = 0;
    };
}

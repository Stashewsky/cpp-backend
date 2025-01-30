#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>
#include <memory>
#include <random>
#include <utility>
#include <iostream>
#include <iterator>

#include "loot_generator.h"
#include "tagged.h"
#include "collision_detector.h"
#include "magic_defs.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

namespace model {
    using namespace std::chrono_literals;
    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    using point = bg::model::point<double, 2, bg::cs::cartesian>;
    using box = bg::model::box<point>;
    using segment = bg::model::segment<point>;
    using value_box = std::pair<box, int>;
    using value_segment = std::pair<segment, int>;

    const double half_road_width = 0.4;

    using Dimension = int;
    using Coord = Dimension;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct CoordD{
        double x = 0.;
        double y = 0.;

        auto operator<=>(const CoordD&) const = default;
    };

    struct SpeedD{
        double x = 0.;
        double y = 0.;

        auto operator<=>(const SpeedD&) const = default;
    };

    enum class Direction{
        NORTH,
        SOUTH,
        WEST,
        EAST,
        STOP
    };

    struct DogData{
        std::pair<double, double> pos;
        std::pair<double, double> speed;
        std::string dir;
        std::string name;
        std::vector<std::pair<int, int>> bag;
        int score;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    class Road {
        struct HorizontalTag {
            HorizontalTag() = default;
        };

        struct VerticalTag {
            VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
                : start_{start}
                , end_{end_x, start.y} {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
                : start_{start}
                , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept
                : bounds_{bounds} {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
                : id_{std::move(id)}
                , position_{position}
                , offset_{offset} {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
                : id_(std::move(id))
                , name_(std::move(name)) {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }

        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }

        const Roads& GetRoads() const noexcept {
            return roads_;
        }

        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }

        void AddOffice(const Office& office);

        std::pair<double, double> GetRandomCoordOnRoads() const;
        size_t GetRandomLootType() const;

        void SetDefaultSpeed(double speed);
        double GetDefaultSpeed() const;

        void SetBagCapacity(int bag_capacity);
        int GetBagCapacity() const;

        void SetCountLootTypes(size_t count_loot);
        [[nodiscard]]size_t GetCountLootTypes() const;

        void AddScoreValue(int value);
        int GetScoreByIdx(int idx) const;

        void CreateRoadRTree();
        //Возвращает результат перемещения собаки по карте дорог, если не успешно, то возвращает координату
        // столкновения с краем дороги.
        std::optional<std::pair<bool, CoordD>> GetSuccessRoadMoveSimple(const CoordD& current_point, const CoordD& move_point) const;

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        bgi::rtree<value_segment, bgi::quadratic<16>> rtree_segment_;
        bgi::rtree< value_box, bgi::quadratic<16> > rtree_box_;

        double map_default_speed_ = 1.0;
        std::unordered_map<size_t, std::pair<double, double>> roads_length_coord_;
        size_t count_loot_types_ = 0;
        int bag_capacity_ = 3;           //default on map
        std::vector<int> score_values_;  //Кол-во очков начисляемых за предмем в lootTypes
    };

    struct BagItem {
        size_t id = 0;
        int type = 0;
    };

    class Dog{
    public:
        explicit Dog(const CoordD& coord, double default_speed, std::string name, size_t max_bag_size):name_(std::move(name)){
            coord_.x = coord.x;
            coord_.y = coord.y;
            default_speed_ = default_speed;
            max_bag_size_ = max_bag_size;
        }

        explicit Dog(const std::pair<double, double>& coord, double default_speed, std::string name, size_t max_bag_size):name_(std::move(name)){
            coord_.x = coord.first;
            coord_.y = coord.second;
            default_speed_ = default_speed;
            max_bag_size_ = max_bag_size;
        }

        [[nodiscard]] std::string GetName() const {return name_;}

        [[nodiscard]] std::pair<double, double> GetCoord() const {
            return {coord_.x, coord_.y};
        }
        [[nodiscard]] CoordD GetCoordD() const {
            return coord_;
        }
        void SetMoveCoord(const CoordD& coord);
        void SetMoveCoord(const std::pair<double, double>& coord);

        void SetDefaultSpeed(double speed);
        double GetDefaultSpeed() const;
        void SetMoveSpeed(const SpeedD& speed);
        void SetMoveSpeed(const std::pair<double, double>& speed);
        [[nodiscard]] std::pair<double, double> GetSpeed() const;
        [[nodiscard]] SpeedD GetSpeedD() const;

        void SetBagCapacity(size_t size);
        [[nodiscard]] size_t GetBagCapacity() const;
        bool SetBagItem(const BagItem& item);
        std::vector<int> ClearBag();
        void SetBagData(const std::vector<BagItem>& bag_items);
        [[nodiscard]] std::vector<BagItem> GetBagData() const;
        void AddScore(int score);
        [[nodiscard]] int GetScore() const;

        void SetRestoreDirection(Direction direction);
        void SetDirection(Direction direction);
        void SetDirection(const std::string& direction);

        [[nodiscard]] Direction GetDirection() const;
        void ApplyTimeTick(size_t time_ms);

        size_t AddLifeTime(size_t delta_time);
        size_t AddStopTime(size_t delta_time);
        void ResetStopTime();

        Dog(const Dog& copy) = default;
        Dog& operator=(const Dog& copy) = default;
        Dog(Dog&& moved) noexcept = default;
        Dog& operator=(Dog&& moved) noexcept = default;

    private:

        std::string name_;
        CoordD coord_;
        SpeedD speed_;
        Direction direct_ = Direction::NORTH;
        double default_speed_ = 1.0;

        std::vector<BagItem> bag_;
        size_t max_bag_size_ = 0;
        int score_ = 0;
        size_t life_time_ms_ = 0;
        size_t stop_time_ms_ = 0;
    };

    struct LootGeneratorConfig{
        double period = 0.0;
        double probability = 0.0;
    };

    struct Loot{
        int type;
        std::pair<double, double> pos;
    };

    class GameSession {
    public:
        GameSession(const Map* map, double period, double probability )
            : map_(map)
            , loot_generator_(loot_gen::LootGenerator::TimeInterval(static_cast<int>(period)), probability) {
        }

        std::size_t AddDog(size_t dog_id, const Dog& dog);
        std::size_t AddDog(size_t dog_id, const std::pair<double, double>& coord, double default_speed,
                           const std::string& name, size_t max_bag_size);
        Dog& GetDogById(std::size_t id);
        size_t GetCountDogs() const;

        const Map* GetMap() const;
        std::vector<std::pair<size_t, std::string>> GetIdAndNames() const;
        std::vector<std::pair<size_t, DogData>> GetIdAndDogDatas() const;

        std::unordered_map<size_t, Dog>& GetAllDogs();

        void GenerateLoot(loot_gen::LootGenerator::TimeInterval time_delta);
        std::unordered_map<size_t, Loot> GetLoot() const;
        void RestoreLoot(const std::unordered_map<size_t, Loot>& loots);

        std::vector<collision_detector::Item> GetLootToItems();
        int GetLootTypeById(int loot_id);
        bool ContainLootById(size_t loot_id);
        void RemoveLootById(size_t loot_id);
        void RemoveDogById(size_t dog_id);

    private:
        std::unordered_map<size_t, Dog> dogs_;
        const Map* map_ = nullptr;
        loot_gen::LootGenerator loot_generator_;
        std::size_t looter_count_ = 0;
        std::size_t loot_id_incr_ = 0;
        std::unordered_map<size_t, Loot> loots_;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        void AddMap(const Map& map);

        const Maps& GetMaps() const noexcept {
            return maps_;
        }

        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        std::shared_ptr<GameSession> FindSession(Map::Id map_id) {
            auto it = std::find_if(sessions_.begin(), sessions_.end(), [&](std::shared_ptr<GameSession> session) {
                auto map = session->GetMap();
                return map_id == map->GetId();
            });
            if(it != sessions_.end()){
                size_t i = std::distance(sessions_.begin(), it);
                return sessions_.at(i);
            }

            return nullptr;
        }

        std::shared_ptr<GameSession> AddNewSession(const Map::Id& map_id);

        void SetDefaultSpeed(double speed);
        double GetDefaultSpeed() const noexcept;
        void SetDefaultBagCapacity(int bag_capacity);
        int GetDefaultBagCapacity() const;
        void SetDefaultRetirementTime(double dog_retirement_time_ms);
        double GetDefaultRetirementTime() const;

        std::vector<std::shared_ptr<GameSession>> GetGameSessions() const;
        void ComputeRoadRTree();

        void SetLootGeneratorConfig(double period, double probability);
        LootGeneratorConfig GetLootGeneratorConfig() const;

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::vector<std::shared_ptr<GameSession>> sessions_;
        double game_default_speed_ = 1.0;
        LootGeneratorConfig loot_generator_config_;
        int default_bag_capacity_ = 3;
        double default_dog_retirement_time_ms_ = 60000.0;  //1 min.
    };
}  // namespace model





























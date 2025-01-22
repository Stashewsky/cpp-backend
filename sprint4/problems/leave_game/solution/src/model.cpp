#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;
using namespace std::chrono_literals;

    std::unordered_map<Direction, std::string> DirectionToString
    {
        {Direction::NORTH, "U"},
        {Direction::SOUTH, "D"},
        {Direction::WEST, "L"},
        {Direction::EAST, "R"},
        {Direction::STOP, ""}
    };

    void Map::AddOffice(const Office& office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } catch (...) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    std::pair<double, double> Map::GetRandomCoordOnRoads() const {

        int nb_roads = static_cast<int>(roads_.size());
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> road_distr(0, nb_roads-1);
        int road_id = road_distr(gen);

        Road road = roads_[road_id];
        Point p1 = road.GetStart();
        int start = 0, end = 0;
        road.IsHorizontal() ? (start = road.GetStart().x, end = road.GetEnd().x)
                            : (start = road.GetStart().y, end = road.GetEnd().y);
        if(start > end) std::swap(start, end);
        std::uniform_int_distribution<> coord_distr(start, end);
        int point = coord_distr(gen);
        return road.IsHorizontal() ? std::make_pair(point, p1.y) : std::make_pair(p1.x, point);
    }

    size_t Map::GetRandomLootType() const {
        int nb_loots = static_cast<int>(count_loot_types_);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, nb_loots-1);
        return distr(gen);
    }

    void Map::CreateRoadRTree() {
        size_t nbRoads = roads_.size();
        std::vector<segment> segments_poly;
        std::vector<box> boxs_poly;
        for(size_t i = 0; i < nbRoads; i++){

            int x0 = roads_[i].GetStart().x;
            int y0 = roads_[i].GetStart().y;
            int x1 = roads_[i].GetEnd().x;
            int y1 = roads_[i].GetEnd().y;

            point p1, p2, p3, p4;
            if(roads_[i].IsHorizontal()){
                if(x0 <= x1){
                    roads_length_coord_.emplace(i, std::pair(x0-half_road_width, x1+half_road_width));
                    p1 = point(x0-half_road_width, y0-half_road_width);
                    p2 = point(x1+half_road_width, y1-half_road_width);
                    p3 = point(x1+half_road_width, y1+half_road_width);
                    p4 = point(x0-half_road_width, y0+half_road_width);
                } else {
                    roads_length_coord_.emplace(i, std::pair(x1-half_road_width, x0+half_road_width));
                    p2 = point(x0+half_road_width, y0-half_road_width);
                    p3 = point(x0+half_road_width, y0+half_road_width);
                    p4 = point(x1-half_road_width, y1+half_road_width);
                    p1 = point(x1-half_road_width, y1-half_road_width);
                }
            } else {   //IsVertical()
                if(y0 <= y1){
                    roads_length_coord_.emplace(i, std::pair(y0-half_road_width, y1+half_road_width));
                    p1 = point(x0-half_road_width, y0-half_road_width);
                    p2 = point(x0+half_road_width, y0-half_road_width);
                    p3 = point(x1+half_road_width, y1+half_road_width);
                    p4 = point(x1-half_road_width, y1+half_road_width);
                } else {
                    roads_length_coord_.emplace(i, std::pair(y1-half_road_width, y0+half_road_width));
                    p4 = point(x0-half_road_width, y0+half_road_width);
                    p1 = point(x1-half_road_width, y1-half_road_width);
                    p2 = point(x1+half_road_width, y1-half_road_width);
                    p3 = point(x0+half_road_width, y0+half_road_width);
                }
            }
            std::vector<segment> segms = {segment(p1, p2), segment(p2, p3), segment(p3, p4), segment(p4, p1)};
            segments_poly.insert(segments_poly.end(), segms.begin(), segms.end());
            boxs_poly.insert(boxs_poly.end(), {box(p1, p3)});
        }

        int segment_increment = 0;
        std::vector<value_segment> value_segments;
        for (const auto& segment : segments_poly) {
            value_segments.push_back(std::pair{segment, segment_increment++});
        }
        rtree_segment_.insert(value_segments.begin(), value_segments.end());

        int box_increment = 0;
        std::vector<value_box> value_boxs;
        for (const auto& box : boxs_poly) {
            value_boxs.push_back(std::pair{box, box_increment});
            box_increment++;
        }
        rtree_box_.insert(value_boxs.begin(), value_boxs.end());
    }

    std::optional<std::pair<bool, CoordD>>
    Map::GetSuccessRoadMoveSimple(const CoordD &current_point, const CoordD &move_point) const {

        point q1(current_point.x, current_point.y);
        std::vector<value_box> result_q1_intersect_box;
        rtree_box_.query(bgi::intersects(q1), std::back_inserter(result_q1_intersect_box));

        std::vector<size_t> roads_contain_q1(result_q1_intersect_box.size());
        std::transform(result_q1_intersect_box.begin(), result_q1_intersect_box.end(), roads_contain_q1.begin(),
                       [](const value_box& v){ return v.second; });

        std::optional<std::pair<bool, CoordD>> result;
        for(auto road_id : roads_contain_q1){
            auto road_length = roads_length_coord_.at(road_id);
            Point road_point_start = roads_.at(road_id).GetStart();
            const double min_border_x = road_point_start.x-half_road_width;
            const double max_border_x = road_point_start.x+half_road_width;
            const double min_border_y = road_point_start.y-half_road_width;
            const double max_border_y = road_point_start.y+half_road_width;

            if(roads_.at(road_id).IsHorizontal()){
                if(move_point.x < road_length.first){
                    result = std::pair(false, CoordD{road_length.first, move_point.y});
                }
                else if(move_point.x > road_length.second){
                    result =  std::pair(false, CoordD{road_length.second, move_point.y});
                }
                else if(move_point.y < min_border_y){
                    result =  std::pair(false, CoordD{move_point.x, min_border_y});
                }
                else if(move_point.y > max_border_y){
                    result =  std::pair(false, CoordD{move_point.x, max_border_y});
                }
                else{
                    return std::pair(true, move_point);
                }
            }
            else{
                if(move_point.y < road_length.first){
                    result = std::pair(false, CoordD{move_point.x, road_length.first});
                }
                else if(move_point.y > road_length.second){
                    result = std::pair(false, CoordD{ move_point.x, road_length.second});
                }
                else if(move_point.x < min_border_x){
                    result = result.has_value() ? result : std::pair(false, CoordD{min_border_x, move_point.y });
                }
                else if(move_point.x > max_border_x){
                    result = result.has_value() ? result : std::pair(false, CoordD{max_border_x, move_point.y});
                }
                else{
                    return std::pair(true, move_point);
                }

            }
        }

        return result;
    }

    void Map::SetCountLootTypes(size_t count_loot) {
        count_loot_types_ = count_loot;
    }

    size_t Map::GetCountLootTypes() const {
        return count_loot_types_;
    }

    void Map::SetDefaultSpeed(double speed) {
        map_default_speed_ = speed;
    }

    double Map::GetDefaultSpeed() const {
        return map_default_speed_;
    }

    void Map::SetBagCapacity(int bag_capacity){
        bag_capacity_ = bag_capacity;
    }

    int Map::GetBagCapacity() const {
        return bag_capacity_;
    }

    void Map::AddScoreValue(int value) {
        score_values_.push_back(value);
    }

    int Map::GetScoreByIdx(int idx) const {
        return score_values_.at(idx);
    }


    void Game::AddMap(const Map& map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } else {
            try {
                maps_.emplace_back(std::move(map));
            } catch (...) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

    std::shared_ptr<GameSession> Game::AddNewSession(const Map::Id &map_id) {
        auto map = FindMap(map_id);
        std::shared_ptr<GameSession> session = std::make_shared<GameSession>( map, loot_generator_config_.period, loot_generator_config_.probability);
        sessions_.push_back(std::move(session));

        return sessions_.back();
    }

    void Game::SetDefaultSpeed(double speed) {
        game_default_speed_ = speed;
    }

    double Game::GetDefaultSpeed() const noexcept {
        return game_default_speed_;
    }

    std::vector<std::shared_ptr<GameSession>> Game::GetGameSessions() {
        return sessions_;
    }

    void Game::ComputeRoadRTree() {
        for(auto& map : maps_){
            map.CreateRoadRTree();
        }
    }

    void Game::SetLootGeneratorConfig(double period, double probability) {
        loot_generator_config_ = {period, probability};
    }

    LootGeneratorConfig Game::GetLootGeneratorConfig() const {
        return  loot_generator_config_;
    }

    void Game::SetDefaultBagCapacity(int bag_capacity) {
        default_bag_capacity_ = bag_capacity;
    }

    int Game::GetDefaultBagCapacity() const {
        return default_bag_capacity_;
    }

    void Game::SetDefaultRetirementTime(double dog_retirement_time_ms) {
        default_dog_retirement_time_ms_ = dog_retirement_time_ms;
    }

    double Game::GetDefaultRetirementTime() const {
        return default_dog_retirement_time_ms_;
    }

    std::size_t GameSession::AddDog(size_t dog_id, const Dog& dog) {
        dogs_.emplace(dog_id, dog);
        looter_count_ = dogs_.size();
        return dog_id;
    }

    std::size_t GameSession::AddDog(size_t dog_id, const std::pair<double, double>& coord, double default_speed,
                                    const std::string &name, size_t max_bag_size) {
        dogs_.emplace(dog_id, Dog(coord, default_speed, name, max_bag_size));
        looter_count_ = dogs_.size();
        return dog_id;
    }

    Dog& GameSession::GetDogById(std::size_t id) {
        return dogs_.at(id);
    }

    size_t GameSession::GetCountDogs() const {
        return dogs_.size();
    }

    const Map* GameSession::GetMap() const {
        return map_;
    }

    std::vector<std::pair<size_t, std::string>> GameSession::GetIdAndNames() const {
        std::vector<std::pair<std::size_t, std::string>> result;
        for(const auto& [id, dog] : dogs_){
            result.emplace_back(id, dog.GetName());
        }

        return result;
    }

    std::vector<std::pair<size_t, DogData>> GameSession::GetIdAndDogDatas() const {
        std::vector<std::pair<size_t, DogData>> result;
        for (const auto& [id, dog] : dogs_) {
            DogData data;
            data.name = dog.GetName();
            data.dir = DirectionToString.at(dog.GetDirection());
            data.pos = dog.GetCoord();
            data.speed = dog.GetSpeed();
            for(const auto& item : dog.GetBagData()){
                data.bag.emplace_back(item.id, item.type);
            }
            data.score = dog.GetScore();
            result.push_back(std::pair{id, data});
        }

        return result;
    }

    std::unordered_map<size_t, Dog>& GameSession::GetAllDogs() {
        return dogs_;
    }

    void GameSession::GenerateLoot(loot_gen::LootGenerator::TimeInterval time_delta) {
        auto nb_loot = loot_generator_.Generate(time_delta,
                                                loots_.size(), looter_count_);
        if(map_ == nullptr){
            throw std::logic_error("GameSession::GenerateLoot() map_ == nullptr");
        }

        for(int i=0; i<nb_loot; i++){
            Loot loot;
            loot.type = map_->GetRandomLootType();
            loot.pos = map_->GetRandomCoordOnRoads();
            loots_.emplace(loot_id_incr_++, std::move(loot));
        }
    }

    std::unordered_map<size_t, Loot> GameSession::GetLoot() const {
        return loots_;
    }

    std::vector<collision_detector::Item> GameSession::GetLootToItems() {
        std::vector<collision_detector::Item> items;
        items.reserve(loots_.size() + 1);
        for(const auto& [id, loot] : loots_){
            items.push_back({{loot.pos.first, loot.pos.second}, Width::ITEM_WIDTH/2, id});
        }

        size_t office_id = 9999;
        auto offices = map_->GetOffices();
        for(const auto& office : offices){
            auto point = office.GetPosition();
            items.push_back({{static_cast<double>(point.x), static_cast<double>(point.y)},
                             Width::OFFICE_WIDTH/2, office_id++});
        }

        return items;
    }

    int GameSession::GetLootTypeById(int loot_id) {
        return loots_.at(loot_id).type;
    }

    bool GameSession::ContainLootById(size_t loot_id) {
        return loots_.contains(loot_id);
    }

    void GameSession::RemoveLootById(size_t loot_id) {
        if(loots_.contains(loot_id)){
            loots_.erase(loot_id);
        }
    }

    void GameSession::RestoreLoot(const std::unordered_map<size_t, Loot> &loots) {
        loots_ = loots;
        loot_id_incr_ = loots_.size();
    }

    void GameSession::RemoveDogById(size_t dog_id) {
        if(dogs_.contains(dog_id)){
            dogs_.erase(dog_id);
        }
        looter_count_ = dogs_.size();
    }

    void Dog::SetMoveCoord(const CoordD& coord) {
        coord_ = coord;
    }

    void Dog::SetMoveCoord(const std::pair<double, double> &coord) {
        coord_.x = coord.first;
        coord_.y = coord.second;
    }

    void Dog::SetMoveSpeed(const SpeedD &speed) {
        speed_ = speed;
    }

    void Dog::SetMoveSpeed(const std::pair<double, double> &speed) {
        speed_.x = speed.first;
        speed_.y = speed.second;
    }

    void Dog::SetRestoreDirection(Direction direction) {
        switch (direction){
            case Direction::NORTH:
                direct_ = Direction::NORTH;
                break;
            case Direction::SOUTH:
                direct_ = Direction::SOUTH;
                break;
            case Direction::WEST:
                direct_ = Direction::WEST;
                break;
            case Direction::EAST:
                direct_ = Direction::EAST;
                break;
            case Direction::STOP:
                direct_ = Direction::STOP;
                break;
        }
    }

    void Dog::SetDirection(Direction direction) {
        switch (direction){
            case Direction::NORTH:
                direct_ = Direction::NORTH;
                speed_ = {0.0, -1 * default_speed_};
                break;
            case Direction::SOUTH:
                direct_ = Direction::SOUTH;
                speed_ = {0.0, default_speed_};
                break;
            case Direction::WEST:
                direct_ = Direction::WEST;
                speed_ = {-1 * default_speed_, 0.0};
                break;
            case Direction::EAST:
                direct_ = Direction::EAST;
                speed_ = {default_speed_, 0.0};
                break;
            case Direction::STOP:
                direct_ = Direction::STOP;
                speed_ = {0.0, 0.0};
                break;
        }
    }

    void Dog::SetDirection(const std::string &direction) {

        if(direction == "U"){
            direct_ = Direction::NORTH;
            speed_ = {0.0, -1 * default_speed_};
        }
        else if(direction == "D"){
            direct_ = Direction::SOUTH;
            speed_ = {0.0, default_speed_};
        }
        else if(direction == "L") {
            direct_ = Direction::WEST;
            speed_ = {-1 * default_speed_, 0.0};
        }
        else if(direction == "R") {
            direct_ = Direction::EAST;
            speed_ = {default_speed_, 0.0};
        }
        else if(direction == "") {
            direct_ = Direction::STOP;
            speed_ = {0.0, 0.0};
        }
        else{
            std::cerr << "Unknown dog direction" << std::endl;
        }
    }

    void Dog::ApplyTimeTick(size_t time_ms) {
        coord_.x += speed_.x * time_ms * 0.001;
        coord_.y += speed_.y * time_ms * 0.001;
    }



    std::pair<double, double> Dog::GetSpeed() const {
        return {speed_.x, speed_.y};
    }

    void Dog::SetDefaultSpeed(double speed) {
        default_speed_ = speed;
    }
    double Dog::GetDefaultSpeed() const {
        return default_speed_;
    }

    void Dog::SetBagCapacity(size_t size) {
        max_bag_size_ = size;
    }

    size_t Dog::GetBagCapacity() const {
        return max_bag_size_;
    }

    bool Dog::SetBagItem(const BagItem &item) {
        if(bag_.size() < max_bag_size_){
            bag_.push_back(item);
            return true;
        }
        return false;
    }

    std::vector<int> Dog::ClearBag() {
        std::vector<int> types;
        for(const auto& item : bag_){
            types.push_back(item.type);
        }
        bag_.clear();

        return types;
    }

    void Dog::AddScore(int score) {
        score_ += score;
    }

    int Dog::GetScore() const {
        return score_;
    }

    void Dog::SetBagData(const std::vector<BagItem> &bag_items) {
        bag_ = bag_items;
    }
    std::vector<BagItem> Dog::GetBagData() const {
        return bag_;
    }

    SpeedD Dog::GetSpeedD() const {
        return speed_;
    }

    Direction Dog::GetDirection() const {
        return direct_;
    }

    size_t Dog::AddLifeTime(size_t delta_time) {
        life_time_ms_ += delta_time;
        return life_time_ms_;
    }

    size_t Dog::AddStopTime(size_t delta_time) {
        stop_time_ms_ += delta_time;
        return stop_time_ms_;
    }

    void Dog::ResetStopTime() {
        stop_time_ms_ = 0;
    }


}  // namespace model

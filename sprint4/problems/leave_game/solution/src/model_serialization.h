#pragma once


#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sstream>
#include <fstream>

#include "model.h"

namespace geom {

    template <typename Archive>
    void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
        ar& point.x;
        ar& point.y;
    }

    template <typename Archive>
    void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
        ar& vec.x;
        ar& vec.y;
    }

}  // namespace geom

namespace model {
    template <typename Archive>
    void serialize(Archive& ar, CoordD& coord, [[maybe_unused]] const unsigned version) {
        ar&(coord.x);
        ar&(coord.y);
    }

    template <typename Archive>
    void serialize(Archive& ar, SpeedD& speed, [[maybe_unused]] const unsigned version) {
        ar&(speed.x);
        ar&(speed.y);
    }

    template <typename Archive>
    void serialize(Archive& ar, Loot& loot, [[maybe_unused]] const unsigned version) {
        ar&(loot.type);
        ar&(loot.pos);
    }

    template <typename Archive>
    void serialize(Archive& ar, BagItem& bag_item, [[maybe_unused]] const unsigned version) {
        ar&(bag_item.id);
        ar&(bag_item.type);
    }



}  // namespace model

namespace serialization {
    class DogRepr {
    public:
        DogRepr() = default;

        explicit DogRepr(size_t dog_id, const model::Dog& dog)
                : id_(dog_id)
                , name_(dog.GetName())
                , coord_(dog.GetCoordD())
                , bag_capacity_(dog.GetBagCapacity())
                , speed_(dog.GetSpeedD())
                , default_speed_(dog.GetDefaultSpeed())
                , direction_(dog.GetDirection())
                , score_(dog.GetScore())
                , bag_content_(dog.GetBagData()) {
        }

        [[nodiscard]] std::pair<size_t, model::Dog> Restore() const {
            model::Dog dog{coord_, default_speed_, name_, bag_capacity_};
            dog.SetMoveSpeed(speed_);
            dog.SetRestoreDirection(direction_);
            dog.AddScore(score_);
            dog.SetBagData(bag_content_);

            return std::pair{id_, dog};
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
            ar& id_;
            ar& name_;
            ar& coord_;
            ar& bag_capacity_;
            ar& speed_;
            ar& default_speed_;
            ar& direction_;
            ar& score_;
            ar& bag_content_;
        }

    private:
        size_t id_ = 0;
        std::string name_;
        model::CoordD coord_;
        size_t bag_capacity_ = 0;
        model::SpeedD speed_;
        double default_speed_ = 1.0;
        model::Direction direction_ = model::Direction::NORTH;
        int score_ = 0;
        std::vector<model::BagItem> bag_content_;
    };





}  // namespace serialization

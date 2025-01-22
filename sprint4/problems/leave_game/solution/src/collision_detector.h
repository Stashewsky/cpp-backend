#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

    struct CollectionResult {
        bool IsCollected(double collect_radius) const;

        // Квадрат расстояния до точки
        double sq_distance;
        // Доля пройденного отрезка
        double proj_ratio;
    };

// Движемся из точки a в точку b и пытаемся подобрать точку c
    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

    struct Item {
        geom::Point2D position;
        double width = 0.0;
        size_t item_id = 0;
    };

    struct Gatherer {
        size_t id = 0;
        geom::Point2D start_pos;
        geom::Point2D end_pos;
        double width = 0.0;
    };

    struct GatheringEvent {
        size_t item_id;
        size_t gatherer_id;
        double sq_distance;
        double time;
    };

    class ItemGathererProvider {
    protected:
        ~ItemGathererProvider() = default;

    public:
        virtual size_t ItemsCount() const = 0;
        virtual Item GetItem(size_t idx) const = 0;
        virtual size_t GatherersCount() const = 0;
        virtual Gatherer GetGatherer(size_t idx) const = 0;
    };

    class VectorItemGathererProvider : public ItemGathererProvider {
    public:
        VectorItemGathererProvider(const std::vector<Item>& items,
                                   const std::vector<Gatherer>& gatherers)
                : items_(items)
                , gatherers_(gatherers) {
        }

        [[nodiscard]] size_t ItemsCount() const override {
            return items_.size();
        }
        [[nodiscard]] Item GetItem(size_t idx) const override {
            return items_.at(idx);
        }
        [[nodiscard]] size_t GatherersCount() const override {
            return gatherers_.size();
        }
        [[nodiscard]] Gatherer GetGatherer(size_t idx) const override {
            return gatherers_.at(idx);
        }

    private:
        std::vector<Item> items_;
        std::vector<Gatherer> gatherers_;
    };

    class CompareEvents {
    public:
        bool operator()(const GatheringEvent& lhs, const GatheringEvent& rhs) {
            if (lhs.gatherer_id != rhs.gatherer_id || lhs.item_id != rhs.item_id){
                return false;
            }
            static const double eps = 1e-10;

            if (std::abs(lhs.sq_distance - rhs.sq_distance) > eps) {
                return false;
            }
            if (std::abs(lhs.time - rhs.time) > eps) {
                return false;
            }
            return true;
        }
    };

    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector
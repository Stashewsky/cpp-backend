#include "collision_detector.h"
#include <cassert>

namespace collision_detector {

    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
        // Проверим, что перемещение ненулевое.
        // Тут приходится использовать строгое равенство, а не приближённое,
        // поскольку при сборе заказов придётся учитывать перемещение даже на небольшое
        // расстояние.
        assert(b.x != a.x || b.y != a.y);
        const double u_x = c.x - a.x;
        const double u_y = c.y - a.y;
        const double v_x = b.x - a.x;
        const double v_y = b.y - a.y;
        const double u_dot_v = u_x * v_x + u_y * v_y;
        const double u_len2 = u_x * u_x + u_y * u_y;
        const double v_len2 = v_x * v_x + v_y * v_y;
        const double proj_ratio = u_dot_v / v_len2;
        const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

        return CollectionResult(sq_distance, proj_ratio);
    }

    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
        std::vector<GatheringEvent> detected_events;

        for (size_t g = 0; g < provider.GatherersCount(); g++) {
            Gatherer gatherer = provider.GetGatherer(g);
            if (geom::IsEqual(gatherer.start_pos, gatherer.end_pos)) {
                continue;
            }
            for (size_t i = 0; i < provider.ItemsCount(); i++) {
                Item item = provider.GetItem(i);
                CollectionResult collect_result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

                if (collect_result.IsCollected(gatherer.width + item.width)) {
                    GatheringEvent evt{.item_id = item.item_id,
                            .gatherer_id = gatherer.id,
                            .sq_distance = collect_result.sq_distance,
                            .time = collect_result.proj_ratio};
                    detected_events.push_back(evt);
                }
            }
        }

        std::sort(detected_events.begin(), detected_events.end(),
                  [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
                      return lhs.time < rhs.time;
                  });

        return detected_events;
    }

    bool CollectionResult::IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }


}  // namespace collision_detector
#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <iostream>
#include <sstream>
#include <cmath>
#include <functional>

// Напишите здесь тесты для функции collision_detector::FindGatherEvents
namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };
}  // namespace Catch

//Testing function
namespace {
    using namespace collision_detector;

    template <typename Range, typename Predicate>
    struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase {
        EqualsRangeMatcher(const Range& range, Predicate predicate)
                : range_{range}
                , predicate_{predicate} {
        }

        template <typename OtherRange>
        bool match(const OtherRange& other) const {
            using std::begin;
            using std::end;

            return std::equal(begin(range_), end(range_), begin(other), end(other), predicate_);
        }

        std::string describe() const override {
            return "Equals: " + Catch::rangeToString(range_);
        }

    private:
        const Range& range_;
        Predicate predicate_;
    };

    template <typename Range, typename Predicate>
    auto EqualsRange(const Range& range, Predicate prediate) {
        return EqualsRangeMatcher<Range, Predicate>{range, prediate};
    }

}

SCENARIO("Collision detection") {
    WHEN("no items") {
        VectorItemGathererProvider provider{
                {}, {{0, {1, 2}, {1, 3}, 0.6},
                                 {1, {0, 0}, {0, 5}, 0.6},
                                 {2, {0, 10}, {-5, 10}, 0.6}}};
        THEN("No events") {
            std::vector<GatheringEvent> events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("no gatherers") {
        VectorItemGathererProvider provider{
                {{{1, 2}, 0.6, 0},
                 {{3, 4}, 0.6, 1},
                 {{-5, 6}, 0.6, 2}}, {}};
        THEN("No events") {
            std::vector<GatheringEvent> events = FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("multiple gatherers and one item") {
        VectorItemGathererProvider provider{{
                                                    {{0, 0}, 0.6, 0},
                                            },
                                            {
                                                    {0, {-5, 0}, {5, 0}, 0.6},
                                                    {1, {0, 1}, {0, -1}, 0.6},
                                                    {2, {-50, 50}, {1, -10}, 0.6},
                                                    {3, {-10, 10}, {11, -10}, 0.6},
                                            }
        };
        THEN("Item gathered by faster gatherer") {
            std::vector<GatheringEvent> events = FindGatherEvents(provider);
            CHECK(events.front().gatherer_id == 3);
        }
    }
    WHEN("Gatherers stands still") {
        VectorItemGathererProvider provider{{
                                                    {{0, 0}, 0.0, 0},
                                            },
                                            {
                                                    {0, {0, 0}, {0, 0}, 0.6},
                                                    {1, {5, 0}, {5, 0}, 0.6},
                                                    {2, {-1, 1}, {-1, 1}, 0.6}
                                            }
        };
        THEN("No events detected") {
            std::vector<GatheringEvent> events = FindGatherEvents(provider);

            CHECK(events.empty());
        }
    }
    WHEN("multiple items on a way of gatherer") {
        VectorItemGathererProvider provider{{
                                                    {{-1, 0}, 0.1, 0},
                                                    {{0, 0.0}, 0.1, 1},
                                                    {{1, 0.03}, 0.1, 2},
                                                    {{2, 0.06}, 0.1, 3},
                                                    {{3, 0.09}, 0.1, 4},
                                                    {{4, 0.12}, 0.1, 5},
                                                    {{5, 0.15}, 0.1, 6},
                                                    {{6, 0.18}, 0.1, 7},
                                                    {{7, 0.21}, 0.1, 8},
                                                    {{8, 0.24}, 0.1, 9},
                                                    {{9, 0.27}, 0.1, 10},
                                            }, {
                                                    {0, {0, 0}, {10, 0}, 0.1},
                                            }};
        THEN("Gathered items in right order") {
            std::vector<GatheringEvent> events = FindGatherEvents(provider);
            CHECK_THAT(
                    events,
                    EqualsRange(std::vector{
                            GatheringEvent{1, 0, 0.*0., 0.0},
                            GatheringEvent{2, 0, 0.03*0.03, 0.1},
                            GatheringEvent{3, 0, 0.06*0.06, 0.2},
                            GatheringEvent{4, 0, 0.09*0.09, 0.3},
                            GatheringEvent{5, 0, 0.12*0.12, 0.4},
                            GatheringEvent{6, 0, 0.15*0.15, 0.5},
                            GatheringEvent{7, 0, 0.18*0.18, 0.6},
                    }, CompareEvents()));
        }
    }
}




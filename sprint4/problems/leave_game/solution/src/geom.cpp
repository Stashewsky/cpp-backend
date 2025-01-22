#include "geom.h"

namespace geom{


    bool CheckCirclesForIntersection(Circle c1, Circle c2) {
        // Вычислим расстояние между точками функцией std::hypot
        return std::hypot(c1.x - c2.x, c1.y - c2.y) <= c1.r + c2.r;
    }

    LineSegment ProjectX(Rect r) {
        return LineSegment{.x1 = r.x, .x2 = r.x + r.w};
    }

    LineSegment ProjectY(Rect r) {
        return LineSegment{.x1 = r.y, .x2 = r.y + r.h};
    }

    std::optional<Rect> Intersect(Rect r1, Rect r2) {
        auto px = Intersect(ProjectX(r1), ProjectX(r2));
        auto py = Intersect(ProjectY(r1), ProjectY(r2));

        if (!px || !py) {
            return std::nullopt;
        }

        // Составляем из проекций прямоугольник
        return Rect{.x = px->x1, .y = py->x1,
                .w = px->x2 - px->x1, .h = py->x2 - py->x1};
    }

    std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2) {
        double left = std::max(s1.x1, s2.x1);
        double right = std::min(s1.x2, s2.x2);

        if (right < left) {
            return std::nullopt;
        }

        return LineSegment{.x1 = left, .x2 = right};
    }

    bool IsEqual(const Point2D &p1, const Point2D &p2) {
        return p1.x == p2.x && p1.y == p2.y;
    }

    Vec2D &Vec2D::operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    Point2D &Point2D::operator+=(const Vec2D &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

}
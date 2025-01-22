#pragma once

#include <vector>
#include <cmath>
#include <optional>
#include <compare>

namespace geom{

    struct Circle {
        double x, y;
        double r;
    };

    [[maybe_unused]] bool CheckCirclesForIntersection(Circle c1, Circle c2);


    struct Rect {
        double x, y;
        double w, h;
    };

    struct LineSegment {
        // Предполагаем, что x1 <= x2
        double x1, x2;
    };

    std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2);

    // Вычисляем проекции на оси
    LineSegment ProjectX(Rect r);

    LineSegment ProjectY(Rect r);

    std::optional<Rect> Intersect(Rect r1, Rect r2);


    struct Vec2D {
        Vec2D() = default;
        Vec2D(double x, double y) : x(x) ,y(y) {}

        Vec2D& operator*=(double scale);
        auto operator<=>(const Vec2D&) const = default;

        double x = 0.0;
        double y = 0.0;
    };

    inline Vec2D operator*(Vec2D lhs, double rhs) {
        return lhs *= rhs;
    }

    inline Vec2D operator*(double lhs, Vec2D rhs) {
        return rhs *= lhs;
    }

    struct Point2D {
        Point2D() = default;
        Point2D(double x, double y) : x(x), y(y) {}

        Point2D& operator+=(const Vec2D& rhs);
        auto operator<=>(const Point2D&) const = default;

        double x = 0.0;
        double y = 0.0;
    };

    inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
        return lhs += rhs;
    }

    inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
        return rhs += lhs;
    }

    [[maybe_unused]] bool IsEqual(const Point2D& p1, const Point2D& p2);

}   //namespace geom






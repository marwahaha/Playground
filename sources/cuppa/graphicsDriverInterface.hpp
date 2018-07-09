#pragma once

#include "unit.hpp"
#include "image.hpp"

#include <string_view>

namespace cuppa
{
    class app;

    struct graphicsDriverInterface
    {
        virtual ~graphicsDriverInterface() = default;

        virtual void setup() = 0;
        virtual void draw(app& _app, DeviceContext _dc) = 0;

        virtual void background(Color color) = 0;
        virtual void noStroke() = 0;
        virtual void stroke(Color color) = 0;
        virtual void stroke(Pixel _thickness) = 0;
        virtual void noFill() = 0;
        virtual void fill(Color color) = 0;
        virtual void point(Point2D pt) = 0;
        virtual void line(Point2D pt1, Point2D pt2) = 0;
        virtual void rect(Point2D center, Move2D size) = 0;
        virtual void ellipse(Point2D center, Move2D size) = 0;
        virtual void arc(Point2D center, Move2D size, Angle start, Angle end, ArcMode mode) = 0;
        virtual void quad(Point2D pt1, Point2D pt2, Point2D pt3, Point2D pt4) = 0;
        virtual void triangle(Point2D pt1, Point2D pt2, Point2D pt3) = 0;
        virtual void text(const char* c, int x, int y) = 0;

        virtual void resetMatrix() = 0;
        virtual void pushMatrix() = 0;
        virtual bool popMatrix() = 0;
        virtual void translate(Move2D translation) = 0;
        virtual void rotate(Angle angle) = 0;
        virtual void scale(float xscale, float yscale, float) = 0;
        virtual void shearX(float slope) = 0;
        virtual void shearY(float slope) = 0;

        virtual Image loadImage(std::string_view filename) = 0;
        virtual void image(const Image& img, Point2D pt) = 0;
    };
}
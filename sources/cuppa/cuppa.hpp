#pragma once

#include "unit.hpp"

namespace cuppa
{
    class app
    {
    public:
        static constexpr float QUARTER_PI = 0.78539816339744830961566084581988f;
        static constexpr float HALF_PI = 1.5707963267948966192313216916398f;
        static constexpr float PI = 3.1415926535897932384626433832795f;
        static constexpr float TWO_PI = 6.283185307179586476925286766559f;

        enum struct ArcMode{ PIE, OPEN, CHORD };

        app() = default;
        virtual ~app() = default;

        virtual void setup() {}
        virtual void update() {}
        virtual void draw() {}

        void run();

    protected:
    // environment
        void size(unsigned int _width, unsigned int _height);

    // colors: settings
        void background(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha);
        void background(unsigned int _red, unsigned int _green, unsigned int _blue)   {   background(_red, _green, _blue, 255); }
        void background(unsigned int _gray, unsigned int _alpha)                      {   background(_gray, _gray, _gray, _alpha); }
        void background(unsigned int _gray)                                           {   background(_gray, _gray, _gray, 255); }

        void noStroke();
        void stroke(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha);
        void stroke(unsigned int _red, unsigned int _green, unsigned int _blue)     {   stroke(_red, _green, _blue, 255);   }
        void stroke(unsigned int _gray)                                             {   stroke(_gray, _gray, _gray, 255);   }
        void strokeWeight(float _thickness);

        void noFill();
        void fill(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha);
        void fill(unsigned int _red, unsigned int _green, unsigned int _blue)   {   fill(_red, _green, _blue, 255); }
        void fill(unsigned int _gray)                                           {   fill(_gray, _gray, _gray, 255); }

    // shapes: 2D primitives
        void rect(int _centerX, int _centerY, unsigned int _width, unsigned int _height);

        void ellipse(int _centerX, int _centerY, unsigned int _width, unsigned int _height) const;
        void ellipse(int _centerX, int _centerY, unsigned int _diameter)        {   ellipse( _centerX, _centerY, _diameter, _diameter ); }

        void text(const char* c, int x, int y);

        void point(Point2D pt);

        void line(int x1, int y1, int x2, int y2);

        void arc(int x, int y, int width, int height, float start_angle, float end_angle, ArcMode mode);
        void arc(int x, int y, int width, int height, float start_angle, float end_angle)   { arc(x, y, width, height, start_angle, end_angle, ArcMode::OPEN); }

        void quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

        void triangle(int x1, int y1, int x2, int y2, int x3, int y3);

    // transforms
        void pushMatrix();
        void popMatrix();
        void resetMatrix();

        void translate(float xmove, float ymove, float zmove);
        void translate(float xscale, float yscale)                  { translate(xscale, yscale, 1.0f); }
        void translate(float _scale)                                { translate(_scale, _scale, _scale); }

        void scale(float xscale, float yscale, float zscale);
        void scale(float xscale, float yscale)                  { scale(xscale, yscale, 1.0f); }
        void scale(float _scale)                                { scale(_scale, _scale, _scale); }

        void rotate(float angle);

        void shearX(float angle);
        void shearY(float angle);
    };
} // namespace wit
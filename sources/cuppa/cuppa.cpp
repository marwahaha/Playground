#include "cuppa.hpp"

#include <cstdlib>
#include <malloc.h>
#include <windows.h>

#include <memory>

#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)
// sample GDI+: https://codes-sources.commentcamarche.net/source/view/29875/966776#browser

template<auto N>
void Assert(bool _condition, const char (&_message)[N])
{
    if(!_condition)
    {
        MessageBox(NULL, _message, "Assertion error!", MB_ICONEXCLAMATION | MB_OK);
        exit(1);
        return;
    }
}

void Assert(bool _condition)
{
    Assert(_condition, "No description");
}

static constexpr char windowClassName[] = "cuppaWindowClass";

template<typename GRAPHICS_DRIVER_TYPE>
struct MSWindowsDriver
{
    void Setup(cuppa::app& _app)
    {
        app_ = &_app;
        instance = this;
        auto hInstance = GetModuleHandle(NULL);
        RegisterWindowClass(hInstance);
        CreateAppWindow(hInstance);
    }

    void SetWindowSize(unsigned int _width, unsigned int _height)
    {
        // for now, we only handle initial width and height
        // TODO: handle SetWindowSize after the creation of the windows
        width = _width;
        height = _height;
    }

    auto& GetGraphicsDriver() { return GraphicsDriver; }

private:
    void Draw()
    {
        PAINTSTRUCT ps;
        auto hdc = BeginPaint(hWnd_, &ps);
        GraphicsDriver.Draw(*app_, hWnd_, hdc);
        EndPaint(hWnd_, &ps);
    }

    void RefreshWindow(HWND _hWnd)
    {
        InvalidateRect(_hWnd, nullptr, TRUE);
        UpdateWindow(_hWnd);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch(msg)
        {
            case WM_CREATE:
                if (instance)
                {
                    instance->GraphicsDriver.Init();
                }
                SetTimer(hwnd, 1234, 16, (TIMERPROC)0);
                break;
            case WM_TIMER:
            // TODO: check the TIMER_ID
                instance->RefreshWindow(hwnd);
                break;
            case WM_PAINT:
                instance->Draw();
                break;
            case WM_CLOSE:
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void RegisterWindowClass(HINSTANCE hInstance)
    {
        WNDCLASSEX wc;
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = 0;
        wc.lpfnWndProc   = WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = windowClassName;
        wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

        if(!RegisterClassEx(&wc))
        {
            MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
            return;
        }
    }

    void CreateAppWindow(HINSTANCE hInstance)
    {
        hWnd_ = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            windowClassName,
            "Cuppa",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            NULL, NULL, hInstance, NULL);
        if (hWnd_ == NULL)
        {
            MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
            return;
        }
        ShowWindow(hWnd_, SW_NORMAL);
        UpdateWindow(hWnd_);
    }

    cuppa::app* app_;

    inline static MSWindowsDriver* instance = nullptr;
    GRAPHICS_DRIVER_TYPE GraphicsDriver;
    HWND hWnd_ { 0 };
    unsigned int width = 240, height = 120;
};

struct GdiplusDriver
{
    void Init()
    {
        Gdiplus::GdiplusStartup(&token, &startupInput, NULL);
        fillBrush_.reset( new SolidBrush{ fillColor_ } );
        stroke_.reset( new Pen{ strokeColor_, strokeWeight_ } );

        font_ = new Font(L"Verdana", 10, Gdiplus::FontStyle::FontStyleRegular, Gdiplus::Unit::UnitPoint);
    }

    void Draw(cuppa::app& _app, HWND /*_hwnd*/, HDC _hdc)
    {
        Assert(graphics_==nullptr);
        Graphics graphics{ _hdc };
        graphics_ = &graphics;
        graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeHighQuality);
        graphics.SetCompositingMode(Gdiplus::CompositingMode::CompositingModeSourceOver);
        graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);

        // zoom x6
        Matrix mx{ 6, 0, 0, 6, 0, 0 };
        graphics.SetTransform(&mx);

        _app.draw();
        graphics_ = nullptr;
    }

    void noStroke()
    {
        strokeEnabled_ = false;
    }

    void stroke(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha)
    {
        strokeEnabled_ = true;
        strokeColor_ = Color{ static_cast<BYTE>(_alpha), static_cast<BYTE>(_red), static_cast<BYTE>(_green), static_cast<BYTE>(_blue) };
        if (strokeEnabled_)
            stroke_.reset( new Pen{ strokeColor_ });
    }

    void strokeWeight(float _thickness)
    {
        strokeEnabled_ = true;
        strokeWeight_ = _thickness;
        stroke_->SetWidth(_thickness);
    }

    void noFill()
    {
        fillEnabled_ = false;
    }

    void fill(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha)
    {
        fillEnabled_ = true;
        fillColor_ = Color{ static_cast<BYTE>(_alpha), static_cast<BYTE>(_red), static_cast<BYTE>(_green), static_cast<BYTE>(_blue) },
        fillBrush_.reset( new SolidBrush( fillColor_ ) );
    }

    void rectangle(int _centerX, int _centerY, int _width, int _height)
    {
        Rect rc{ _centerX - _width / 2, _centerY - _height / 2, _width, _height };
        if (fillEnabled_)
            graphics_->FillRectangle( fillBrush_.get(), rc );
        if (strokeEnabled_)
            graphics_->DrawRectangle( stroke_.get(), rc );
    }

    void ellipse(int _centerX, int _centerY, int _width, int _height)
    {
        Rect rc{ _centerX - _width / 2, _centerY - _height / 2, _width, _height };
        if (fillEnabled_)
            graphics_->FillEllipse( fillBrush_.get(), rc );
        if (strokeEnabled_)
            graphics_->DrawEllipse( stroke_.get(), rc );
    }

    void text(const char* c, int x, int y)
    {
        auto len = strlen(c);
        auto wcs = (WCHAR*)_malloca((len+1)*sizeof(WCHAR));
        decltype(len) ret;
        mbstowcs_s(&ret, wcs, len + 1, c, len+1);
        graphics_->DrawString(wcs, static_cast<INT>(len), font_, PointF{static_cast<REAL>(x), static_cast<REAL>(y)}, fillBrush_.get());
    }

    void point(int x, int y, int /*z*/)
    {
        RectF rc{ x-0.5f, y-0.5f, 1, 1 };
        SolidBrush b{ strokeColor_ };
        graphics_->FillRectangle( &b, rc );
    }

    void line(int x1, int y1, int /*z1*/, int x2, int y2, int /*z2*/)
    {
        if (strokeEnabled_)
            graphics_->DrawLine( stroke_.get(), x1, y1, x2, y2 );
    }

    using ArcMode = cuppa::app::ArcMode;
    void arc(int x, int y, int width, int height, float start_angle, float end_angle, ArcMode mode)
    {
        Rect rc{ x - width / 2, y - height / 2, width, height };
        auto sweep_angle = end_angle - start_angle;
        if (mode==ArcMode::PIE)
        {
            if (fillEnabled_)
                graphics_->FillPie( fillBrush_.get(), rc, start_angle, sweep_angle );
            if (strokeEnabled_)
                graphics_->DrawPie( stroke_.get(), rc, start_angle, sweep_angle );
        }
        else
        {
            GraphicsPath path;

            path.AddArc(rc, start_angle, sweep_angle);
            if (mode==ArcMode::CHORD)
            {
                path.CloseFigure();
            }
            if (fillEnabled_)
                graphics_->FillPath( fillBrush_.get(), &path );
            if (strokeEnabled_)
                graphics_->DrawPath( stroke_.get(), &path);
        }
    }

    void quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
    {
        GraphicsPath path;

        path.AddLine( x1, y1, x2, y2 );
        path.AddLine( x3, y3, x4, y4 );
        path.CloseFigure();

        if (fillEnabled_)
            graphics_->FillPath( fillBrush_.get(), &path );
        if (strokeEnabled_)
            graphics_->DrawPath( stroke_.get(), &path);
    }

    void triangle(int x1, int y1, int x2, int y2, int x3, int y3)
    {
        GraphicsPath path;

        path.AddLine( x1, y1, x2, y2 );
        path.AddLine( x2, y2, x3, y3 );
        path.CloseFigure();

        if (fillEnabled_)
            graphics_->FillPath( fillBrush_.get(), &path );
        if (strokeEnabled_)
            graphics_->DrawPath( stroke_.get(), &path);
    }

private:
    using Graphics = Gdiplus::Graphics;
    using REAL = Gdiplus::REAL;
    using ARGB = Gdiplus::ARGB;
    using Matrix = Gdiplus::Matrix;
    using Color = Gdiplus::Color;
    using Pen = Gdiplus::Pen;
    using Brush = Gdiplus::Brush;
    using SolidBrush = Gdiplus::SolidBrush;
    using Point = Gdiplus::Point;
    using PointF = Gdiplus::PointF;
    using Rect = Gdiplus::Rect;
    using RectF = Gdiplus::RectF;
    using Font = Gdiplus::Font;
    using FontFamily = Gdiplus::FontFamily;
    using GraphicsPath = Gdiplus::GraphicsPath;

    Graphics*       graphics_ = nullptr;

    std::unique_ptr<Brush>  fillBrush_;
    Color                   fillColor_ = (ARGB)Color::White;
    bool                    fillEnabled_ = true;

    std::unique_ptr<Pen>    stroke_;
    float                   strokeWeight_ = 1.0f;
    Color                   strokeColor_ = (ARGB)Color::Black;
    bool                    strokeEnabled_ = true;

    Font*           font_ = nullptr;

    ULONG_PTR                       token;
    Gdiplus::GdiplusStartupInput    startupInput;
};

static MSWindowsDriver<GdiplusDriver> SystemDriver;

namespace cuppa
{
    void app::run()
    {
        // __argc, __argv
        setup();
        SystemDriver.Setup(*this);
        MSG Msg;
        while(GetMessage(&Msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
            update();
        }
    }

    void app::size(unsigned int _width, unsigned int _height)
    {
        SystemDriver.SetWindowSize(_width, _height);
    }

    void app::noStroke()
    {
        SystemDriver.GetGraphicsDriver().noStroke();
    }

    void app::stroke(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha)
    {
        SystemDriver.GetGraphicsDriver().stroke(_red, _green, _blue, _alpha);
    }

    void app::strokeWeight(float _thickness)
    {
        SystemDriver.GetGraphicsDriver().strokeWeight(_thickness);
    }

    void app::noFill()
    {
        SystemDriver.GetGraphicsDriver().noFill();
    }

    void app::fill(unsigned int _red, unsigned int _green, unsigned int _blue, unsigned int _alpha)
    {
        SystemDriver.GetGraphicsDriver().fill(_red, _green, _blue, _alpha);
    }

    void app::rectangle(int _centerX, int _centerY, unsigned int _width, unsigned int _height)
    {
        SystemDriver.GetGraphicsDriver().rectangle( _centerX, _centerY, _width, _height );
    }

    void app::ellipse(int _centerX, int _centerY, unsigned int _width, unsigned int _height) const
    {
        SystemDriver.GetGraphicsDriver().ellipse( _centerX, _centerY, _width, _height );
    }

    void app::text(const char* c, int x, int y)
    {
        SystemDriver.GetGraphicsDriver().text( c, x, y);
    }

    void app::point(int x, int y)
    {
        SystemDriver.GetGraphicsDriver().point(x, y, 0);
    }

    void app::line(int x1, int y1, int x2, int y2)
    {
        SystemDriver.GetGraphicsDriver().line( x1, y1, 0, x2, y2, 0);
    }

    void app::arc(int x, int y, int width, int height, float start_angle, float end_angle, ArcMode mode)
    {
        SystemDriver.GetGraphicsDriver().arc( x, y, width, height, start_angle / PI * 180, end_angle / PI * 180, mode);
    }

    void app::quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
    {
        SystemDriver.GetGraphicsDriver().quad( x1, y1, x2, y2, x3, y3, x4, y4);
    }

    void app::triangle(int x1, int y1, int x2, int y2, int x3, int y3)
    {
        SystemDriver.GetGraphicsDriver().triangle( x1, y1, x2, y2, x3, y3);
    }
} // namespace cuppa
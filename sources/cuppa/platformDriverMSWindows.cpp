#include "platformDriverInterface.hpp"

#include "cuppa.hpp" // for cuppa::app

#include <windows.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#include <Xinput.h>
#include <iostream>

namespace cuppa
{
    gamepad nullGamepad{ false, { 0_px, 0_px }, { 0_px, 0_px } };
    gamepad gamepads[XUSER_MAX_COUNT];

    void xinput_update()
    {
        DWORD dwResult;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE state;
            ZeroMemory( &state, sizeof(XINPUT_STATE));

            dwResult = XInputGetState( i, &state );
            if (dwResult == ERROR_SUCCESS)
            {
                gamepads[i].connected = true;
                gamepads[i].LeftPad = { Pixel{ state.Gamepad.sThumbLX / 32767.f}, Pixel{ state.Gamepad.sThumbLY / 32767.f } };
                gamepads[i].RightPad = { Pixel{ state.Gamepad.sThumbRX / 32767.f}, Pixel{ state.Gamepad.sThumbRY / 32767.f } };
            }
            else
            {
                gamepads[i].connected = false;
            }
        }
    }

    static constexpr TCHAR windowClassName[] = TEXT("cuppaWindowClass");

    void assertionError(const char* _message)
    {
        MessageBoxA(NULL, _message, "Assertion error!", MB_ICONEXCLAMATION | MB_OK);
    }

    struct platformDriverMSWindows : platformDriverInterface
    {
        platformDriverMSWindows() = default;
        explicit platformDriverMSWindows(HWND _hWnd) : hWnd_{_hWnd} {}

        static auto getAppPtr(HWND hWnd) { return reinterpret_cast<app*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)); }

        void setup(app& _app) override
        {
            auto hInstance = GetModuleHandle(NULL);
            registerWindowClass(hInstance);
            createAppWindow(hInstance, _app);
        }

        void run() override
        {
            auto appPtr = getAppPtr(hWnd_);
            MSG Msg;
            while(GetMessage(&Msg, NULL, 0, 0) > 0)
            {
                appPtr->update();
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }

        void size(Pixel /*_width*/, Pixel /*_height*/) override
        {
            // for now, we only handle initial width and height
            // TODO: handle SetWindowSize after the creation of the windows
        }

        const gamepad& getGamepad(std::size_t padIndex) override
        {
            return gamepads[padIndex];
        }

private:
        void draw()
        {
            auto appPtr = getAppPtr(hWnd_);
            POINT p;
            if (GetCursorPos(&p))
            {
                if (ScreenToClient(hWnd_, &p))
                {
                    appPtr->setMousePosition({Pixel{p.x}, Pixel{p.y}});
                }
            }
            PAINTSTRUCT ps;
            auto hdc = BeginPaint(hWnd_, &ps);
            //auto appPtr = getAppPtr(hWnd_);
            auto gfxDvrItf = appPtr->graphicsDriver.get();
            gfxDvrItf->draw(*appPtr, DeviceContext{hdc});
            EndPaint(hWnd_, &ps);
        }

        void refreshWindow()
        {
            InvalidateRect(hWnd_, nullptr, FALSE);
            UpdateWindow(hWnd_);
            xinput_update();
        }

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto platformDriver = platformDriverMSWindows{ hwnd };
            switch(msg)
            {
                case WM_CREATE:     platformDriver.onCreate();                                             break;
                case WM_TIMER:      platformDriver.onTimer(wParam);                                        break;
                case WM_PAINT:      platformDriver.onPaint();                                              break;
                case WM_CLOSE:      platformDriver.onClose();                                              break;
                case WM_DESTROY:    platformDriver.onDestroy();                                            break;
                case WM_SIZE:       platformDriver.onSize({Pixel(LOWORD(lParam)), Pixel(HIWORD(lParam))}); break;
                case WM_LBUTTONDOWN:platformDriver.onLeftMouseButtonDown({Pixel((lParam)), Pixel(GET_Y_LPARAM(lParam))});    break;
                case WM_LBUTTONUP:  platformDriver.onLeftMouseButtonUp({Pixel((lParam)), Pixel(GET_Y_LPARAM(lParam))});      break;
                //case WM_MOUSEMOVE:  platformDriver.onMouseMove({Pixel(LOWORD(lParam)), Pixel(HIWORD(lParam))}); break;
                default:            return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            return 0;
        }

        void onCreate()
        {
            SetTimer(hWnd_, 1234, 16, (TIMERPROC)0);
        }

        void onClose()
        {
            DestroyWindow(hWnd_);
        }

        void onDestroy()
        {
            PostQuitMessage(0);
        }

        void onTimer(UINT_PTR timerID)
        {
            // TODO: check the TIMER_ID
            if (timerID==1234)
            {
                refreshWindow();
            }
        }

        void onPaint()
        {
            draw();
        }

        void onSize(Direction newSize)
        {
            auto appPtr = getAppPtr(hWnd_);
            appPtr->setSize(newSize);
        }

        //Point leftMouseButtonDownPosition;
        void onLeftMouseButtonDown(Point /*position*/)
        {
            //leftMouseButtonDownPosition = position;
        }

        void onLeftMouseButtonUp(Point position)
        {
            //if (leftMouseButtonDownPosition == position)
            {
                getAppPtr(hWnd_)->mouseClick(position);
            }
        }

        // void onMouseMove(Point position)
        // {
        //     getAppPtr(hWnd_)->mouseMove(position);
        // }

        void registerWindowClass(HINSTANCE hInstance)
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
                MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
                return;
            }
        }

        void createAppWindow(HINSTANCE hInstance, app& _app)
        {
            RECT rc{0,0, _app.getWidth().getAs<int>(), _app.getHeight().getAs<int>()};
            AdjustWindowRectEx(&rc,WS_OVERLAPPEDWINDOW,FALSE, WS_EX_CLIENTEDGE);
            hWnd_ = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                windowClassName,
                TEXT("Cuppa"),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
                NULL, NULL, hInstance, NULL);
            if (hWnd_ == NULL)
            {
                MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
                return;
            }
            SetLastError(0);
            if (SetWindowLongPtr(hWnd_, GWLP_USERDATA, (LONG_PTR)&_app)==0 && GetLastError()!=0)
            {
                MessageBox(NULL, TEXT("SetWindowLongPtr 1 Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
                return;
            }
            auto p = getAppPtr(hWnd_);
            Assert(p==&_app);
            ShowWindow(hWnd_, SW_NORMAL);
            UpdateWindow(hWnd_);
        }

        HWND hWnd_ { 0 };
    };

    std::unique_ptr<platformDriverInterface> createPlatformDriverMSWindows() { return std::make_unique<platformDriverMSWindows>(); }
} // namespace cuppa
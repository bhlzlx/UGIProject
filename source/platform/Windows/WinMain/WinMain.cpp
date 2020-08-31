#include <windows.h>
#include "Resource.h"
#include <UGIApplication.h>
#include <hgl/assets/AssetsSource.h>
#include <hgl/CodePage.h>
#include <string>
#include <regex>
#include <cmath>

#define MAX_LOADSTRING 100

static int captionHeight;
static int systemMenuHeight;
static int frameThicknessX;
static int frameThicknessY;

HINSTANCE hInst;                                
WCHAR szTitle[MAX_LOADSTRING];                  
WCHAR szWindowClass[MAX_LOADSTRING];            

UGIApplication* object = nullptr;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain
(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EMPLTYWINDOW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    FARPROC spdpia = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetProcessDPIAware");
#ifndef NODPI
    if (spdpia != NULL) spdpia(); // 去掉这一句可看到DPI缩放效果
#endif


    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CON", "r", stdin);//重定向输入流
    freopen_s(&stream, "CON", "w", stdout);//重定向输入流
    
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }


    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EMPLTYWINDOW));
    //
    MSG msg;
/* program main loop */
    bool bQuit = false;
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            object->tick();
            //eglSwapBuffers(context.display, context.surface);
            Sleep(0);
        }
    }

    FreeConsole();

    return (int)msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EMPLTYWINDOW));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_EMPLTYWINDOW);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    captionHeight = GetSystemMetrics(SM_CYCAPTION);
    systemMenuHeight = GetSystemMetrics(SM_CYMENU);
    frameThicknessX = GetSystemMetrics(SM_CXSIZEFRAME);
    frameThicknessY = GetSystemMetrics(SM_CYSIZEFRAME);

    RECT wndRc = {
        0, 0, 640, 480
    };
    AdjustWindowRect( &wndRc, WS_OVERLAPPEDWINDOW, TRUE);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wndRc.right, wndRc.bottom, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }



    object = GetApplication();

    char pathBuff[256];
    GetModuleFileNameA(NULL, pathBuff, 256);
    std::string assetRoot = "./";
    const char expr[] = R"(^(.*)\\[_0-9a-zA-Z]*.exe$)";
    std::regex pathRegex(expr);
    std::smatch result;
    std::string content(pathBuff);
    bool succees = std::regex_match(content, result, pathRegex);
    if (succees) {
        assetRoot = result[1];
    }
    assetRoot.append("/../");
    auto assetsSource = hgl::assets::CreateSourceByFilesystem( "achive", hgl::ToOSString(assetRoot.c_str()), true );
    // auto archieve = kwheel::CreateStdArchieve( assetRoot );
    if (!object->initialize(hWnd, assetsSource)) {
        return FALSE;
    }
    //
    HDC hdcScreen = GetDC(NULL);
    int iX = GetDeviceCaps(hdcScreen, HORZRES);    // pixel
    int iY = GetDeviceCaps(hdcScreen, VERTRES);    // pixel
    int iPhsX = GetDeviceCaps(hdcScreen, HORZSIZE);    // mm
    int iPhsY = GetDeviceCaps(hdcScreen, VERTSIZE);    // mm
    if (NULL != hdcScreen){
        DeleteDC(hdcScreen);
    }
#define INCH 0.03937
    float iTemp = iPhsX * iPhsX + iPhsY * iPhsY;
    float fInch = sqrt(iTemp) * INCH;
    iTemp = iX * iX + iY * iY;
    float fPixel = sqrt(iTemp);

    float iDPI = fPixel / fInch;    // dpi pixel/inch
    //
    SetWindowTextA(hWnd, object->title());
    //
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case WM_KEYDOWN:
        {
            object->onKeyEvent(wParam, UGIApplication::eKeyDown);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        object->release();
        PostQuitMessage(0);
        break;
    case WM_SIZE:
    {
        UINT cx = LOWORD(lParam);
        UINT cy = HIWORD(lParam);
        //::GetClientRect();
        object->resize(cx, cy);
        break;
    }
    case WM_KEYDOWN:
    {
        object->onKeyEvent(wParam, UGIApplication::eKeyDown);
        break;
    }
    case WM_KEYUP:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
        {
            //PostQuitMessage(0);
            break;
        }
        default:
            object->onKeyEvent(wParam, UGIApplication::eKeyUp);
            break;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        short x = LOWORD(lParam);
        short y = HIWORD(lParam);
        switch (wParam)
        {
        case MK_LBUTTON:
            object->onMouseEvent(UGIApplication::LButtonMouse, UGIApplication::MouseMove, x, y);
            break;
        case MK_RBUTTON:
            object->onMouseEvent(UGIApplication::RButtonMouse, UGIApplication::MouseMove, x, y);
            break;
        case MK_MBUTTON:
            object->onMouseEvent(UGIApplication::MButtonMouse, UGIApplication::MouseMove, x, y);
            break;
        }
        break;
    }
    case WM_RBUTTONDOWN:
    {
        short x = LOWORD(lParam);
        short y = HIWORD(lParam);
        object->onMouseEvent(UGIApplication::RButtonMouse, UGIApplication::MouseDown, x, y);
        break;
    }
    case WM_RBUTTONUP:
    {
        short x = LOWORD(lParam);
        short y = HIWORD(lParam);
        object->onMouseEvent(UGIApplication::RButtonMouse, UGIApplication::MouseUp, x, y);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

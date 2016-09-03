#include "stdafx.h"
#include "Window.h"

std::unordered_map<HWND, dmp::Window*> dmp::Window::windowMap;

dmp::Window::Window(HINSTANCE hInstance, int width, int height, std::wstring title)
   : mWidth(width)
   , mHeight(height)
   , mTitle(title)
   , mhAppInst(hInstance)
{
   expectTrue("create window", init());
   mRenderer = std::make_unique<Renderer>(mhMainWnd, mWidth, mHeight);
}


dmp::Window::~Window()
{}

dmp::Window * dmp::Window::getByHWND(HWND key)
{
   auto i = windowMap.find(key);

   if (i != windowMap.end()) return i->second;

   return nullptr;
}

LRESULT dmp::Window::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_SIZE:
         mWidth = LOWORD(lParam);
         mHeight = HIWORD(lParam);
         //if (md3dDevice)
         //{
         //   if (wParam == SIZE_MINIMIZED)
         //   {
         //      mAppPaused = true;
         //      mMinimized = true;
         //      mMaximized = false;
         //   }
         //   else if (wParam == SIZE_MAXIMIZED)
         //   {
         //      mAppPaused = false;
         //      mMinimized = false;
         //      mMaximized = true;
         //      OnResize();
         //   }
         //   else if (wParam == SIZE_RESTORED)
         //   {

         //      // Restoring from minimized state?
         //      if (mMinimized)
         //      {
         //         mAppPaused = false;
         //         mMinimized = false;
         //         OnResize();
         //      }

         //      // Restoring from maximized state?
         //      else if (mMaximized)
         //      {
         //         mAppPaused = false;
         //         mMaximized = false;
         //         OnResize();
         //      }
         //      else if (mResizing)
         //      {
         //         // If user is dragging the resize bars, we do not resize 
         //         // the buffers here because as the user continuously 
         //         // drags the resize bars, a stream of WM_SIZE messages are
         //         // sent to the window, and it would be pointless (and slow)
         //         // to resize for each WM_SIZE message received from dragging
         //         // the resize bars.  So instead, we reset after the user is 
         //         // done resizing the window and releases the resize bars, which 
         //         // sends a WM_EXITSIZEMOVE message.
         //      }
         //      else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
         //      {
         //         OnResize();
         //      }
         //   }
         //}
         return 0;
         // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
      //case WM_ENTERSIZEMOVE:
      //   mAppPaused = true;
      //   mResizing = true;
      //   mTimer.Stop();
      //   return 0;

      //   // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
      //   // Here we reset everything based on the new window dimensions.
      //case WM_EXITSIZEMOVE:
      //   mAppPaused = false;
      //   mResizing = false;
      //   mTimer.Start();
      //   OnResize();
      //   return 0;
      // WM_DESTROY is sent when the window is being destroyed.
      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
         // Catch this message so to prevent the window from becoming too small.
      case WM_GETMINMAXINFO:
         ((MINMAXINFO*) lParam)->ptMinTrackSize.x = 200;
         ((MINMAXINFO*) lParam)->ptMinTrackSize.y = 200;
         return 0;
      default: 
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   auto w = dmp::Window::getByHWND(hwnd);
   if (w != nullptr)
   {
      return w->wndProc(hwnd, msg, wParam, lParam);
   }
   else
   {
      return DefWindowProc(hwnd, msg, wParam, lParam);
   }
}

bool dmp::Window::init()
{
   WNDCLASS wc;
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = MainWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = mhAppInst; 
   wc.hIcon = LoadIcon(0, IDI_APPLICATION);
   wc.hCursor = LoadCursor(0, IDC_ARROW);
   wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
   wc.lpszMenuName = 0;
   wc.lpszClassName = L"MainWnd";

   if (!RegisterClass(&wc))
   {
      MessageBox(0, L"RegisterClass Failed.", 0, 0);
      return false;
   }

   // Compute window rectangle dimensions based on requested client area dimensions.
   RECT R = {0, 0, mWidth, mHeight};
   AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
   int width = R.right - R.left;
   int height = R.bottom - R.top;

   mhMainWnd = CreateWindow(L"MainWnd", mTitle.c_str(),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
   if (!mhMainWnd)
   {
      MessageBox(0, L"CreateWindow Failed.", 0, 0);
      return false;
   }

   windowMap.insert({mhMainWnd, this});

   ShowWindow(mhMainWnd, SW_SHOW);
   UpdateWindow(mhMainWnd);

   return true;
}

int dmp::Window::run()
{
   MSG msg = {0};

   //mTimer.Reset();

   while (msg.message != WM_QUIT)
   {
      // If there are Window messages then process them.
      if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      // Otherwise, do animation/game stuff.
      else
      {
         //mTimer.Tick();

         //if (!mAppPaused)
         //{
         //   CalculateFrameStats();
         //   Update(mTimer);
         //   Draw(mTimer);
         //}
         //else
         //{
         //   Sleep(100);
         //}
      }
   }

   return (int) msg.wParam;
}
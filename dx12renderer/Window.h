#pragma once

#include "Renderer.h"

namespace dmp
{
   class Window
   {
   public:
      Window(HINSTANCE hInstance, int width, int height, std::wstring title);
      ~Window();

      
      LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      int run();

      static Window * getByHWND(HWND key);
      static std::unordered_map<HWND, Window*> windowMap;
   private:
      bool init();

      int mWidth = 800;
      int mHeight = 600;
      std::wstring mTitle = L"Noonoo disapproves of win32";
      HWND mhMainWnd = nullptr;
      HINSTANCE mhAppInst = nullptr;

      std::unique_ptr<Renderer> mRenderer = nullptr;
      
   };

}

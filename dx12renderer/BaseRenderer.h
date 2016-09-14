#pragma once

#include "Timer.h"

namespace dmp
{
   class BaseRenderer
   {
   public:
      BaseRenderer() {}
      BaseRenderer(HWND windowHandle, int width, int height);
      virtual ~BaseRenderer();

      float getAspectRatio() const
      {
         expectTrue("rhs not 0",
                    mHeight != 0);
         return static_cast<float>(mWidth) / static_cast<float>(mHeight);
      }

      bool isMsaaEnabled() const { return mMsaaEnabled; }
      void setMsaaState(bool state) { mMsaaEnabled = state; }

      HRESULT resize(int width, int height, bool force = false);

      void draw();

      void update(const Timer & t);

   protected:

      // update hooks
      virtual bool updatePre(const Timer & t) { return true; }
      virtual bool updateImpl(const Timer & t) { return true; }
      virtual bool updatePost(const Timer & t) { return true; }

      // resize hooks
      virtual HRESULT resizePre(int width, int height, bool force = false) { return S_OK; }
      virtual HRESULT resizeImpl(int width, int height, bool force = false);
      virtual HRESULT resizePost(int width, int height, bool force = false) { return S_OK; }

      // draw hooks
      virtual HRESULT drawPre() { return S_OK; }
      virtual HRESULT drawImpl() = 0;
      virtual HRESULT drawPost() { return S_OK; }

      // derived classes should call this in their constructor
      void init();

      // this will be called by init and need not be called by the derived class
      virtual bool initPre() { return true; }
      virtual bool initImpl() = 0;
      virtual bool initPost() { return true; }

      HRESULT recreateSwapChain();

      bool createDescriptorHeaps(); // TODO: candidate for virtual

      // fields

      void flushCommandQueue(std::function<void()> callback = doNothing);
      ID3D12Resource * currentBackBuffer();
      D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() const;
      D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView() const;

      HWND mWindowHandle;
      int mWidth;
      int mHeight;

      Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
      Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
      uint64_t mFenceVal = 0;
      Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

      UINT mRtvDescriptorSize = 0;
      UINT mDsvDescriptorSize = 0;
      UINT mCbvSrvUavDescriptorSize = 0;

      DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      UINT mMsaaQualityLevel = 0;
      bool mMsaaEnabled = false;

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

      Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
      UINT mCurrentBackBuffer = 0;
      std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SWAP_CHAIN_BUFFER_COUNT> mSwapChainBuffers;

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

      DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
      D3D12_VIEWPORT mViewport;
      D3D12_RECT mScissorRect;

   private:
      bool mReady = false;
      bool initBase();
      bool initMsaa();
      bool initCommand();
 };
}
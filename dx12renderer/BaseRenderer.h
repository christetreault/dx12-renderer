#pragma once

namespace dmp
{
   class BaseRenderer
   {
   public:
      BaseRenderer() = delete;
      BaseRenderer(HWND windowHandle, int width, int height);
      ~BaseRenderer();

      float getAspectRatio() const
      {
         expectTrue("rhs not 0",
                    mHeight != 0);
         return static_cast<float>(mWidth) / static_cast<float>(mHeight);
      }

      bool isMsaaEnabled() const { return mMsaaEnabled; }
      void setMsaaState(bool state) { mMsaaEnabled = state; }
      HRESULT resize(int width, int height, bool force = false); // TODO: W I P

      void draw(/* TODO */);

      /* the renderer shouldn't know about updating geometry. Need a
         separate Geometry class of some sort that can update itself.
         (would this be the renderItem thing?) */

      // TODO: initialization W I P
   private:
      void flushCommandQueue(std::function<void()> callback = doNothing);
      ID3D12Resource * currentBackBuffer();
      D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() const;
      D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView() const;

      HWND mWindowHandle;
      int mWidth;
      int mHeight;

      bool initBase();

      Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
      Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
      uint64_t mFenceVal = 0;
      Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

      UINT mRtvDescriptorSize = 0;
      UINT mDsvDescriptorSize = 0;
      UINT mCbvSrvUavDescriptorSize = 0;

      bool initMsaa();

      DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      UINT mMsaaQualityLevel = 0;
      bool mMsaaEnabled = false;

      bool initCommand();

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

      HRESULT recreateSwapChain();

      Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
      UINT mCurrentBackBuffer = 0;
      std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SWAP_CHAIN_BUFFER_COUNT> mSwapChainBuffers;

      bool createDescriptorHeaps(); // TODO: W I P

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

      DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
      D3D12_VIEWPORT mViewport;
      D3D12_RECT mScissorRect;
   };
}
#pragma once

#include "BaseRenderer.h"
#include "BasicTypes.h"
#include "Mesh.h"
#include "FrameResource.h"

namespace dmp
{
   class BasicRenderer : public BaseRenderer
   {
   public:
      BasicRenderer(HWND windowHandle, int width, int height) :
         BaseRenderer(windowHandle, width, height)
      {
         init();
      }

      BasicRenderer();
      ~BasicRenderer();
   private:
      HRESULT drawImpl();
      HRESULT drawPre();
      HRESULT drawPost();
      bool initImpl();

      bool buildRootSignature();
      Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;

      bool buildShadersAndInputLayouts();
      std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
      const std::string vertexShaderFile = "BasicVertexShader.cso";
      const std::string pixelShaderFile = "BasicPixelShader.cso";
      std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
      
      bool buildMeshBuffer();
      std::unique_ptr<MeshBuffer<BasicVertex, uint16_t>> mMeshBuffer = nullptr;

      bool buildRenderItems(); // just one today
      std::vector<std::unique_ptr<BasicRenderItem>> mVger;

      bool buildFrameResources();
      std::vector<std::unique_ptr<FrameResource<BasicPassConstants, BasicObjectConstants>>> mFrameResources;

      bool buildCBVDescriptorHeaps();
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBVHeap = nullptr;
      UINT mPassCBVOffset = 0;

      bool buildConstantBufferViews();

      bool buildPSOs();
      std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
      const std::string opaqueKey = "opaque";
      const std::string wireframeKey = "wireframe";

      bool updatePre(const Timer & t) override;
      FrameResource<BasicPassConstants, BasicObjectConstants> * mCurrFrameResource = nullptr;
      int mCurrFrameResourceIndex = 0;

      bool updateImpl(const Timer & t) override;
      DirectX::XMFLOAT4 mClearColor;

      HRESULT resizeImpl(int width, int height, bool force = false) override;
      DirectX::SimpleMath::Matrix mP;
      DirectX::SimpleMath::Matrix mV;
   };

}
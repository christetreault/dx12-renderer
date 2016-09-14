#pragma once
#include "stdafx.h"
#include "Mesh.h"

namespace dmp
{
   struct BasicVertex
   {
      DirectX::XMFLOAT4 pos;
      DirectX::XMFLOAT4 normal;
   };

   struct BasicPassConstants
   {
      DirectX::XMFLOAT4X4 V;
      DirectX::XMFLOAT4X4 inverseV;
      DirectX::XMFLOAT4X4 P;
      DirectX::XMFLOAT4X4 inverseP;
      DirectX::XMFLOAT4X4 PV;
      DirectX::XMFLOAT4X4 inversePV;
      DirectX::XMFLOAT4 eye;
      //DirectX::XMFLOAT2 RenderTargetSize = {0.0f, 0.0f};
      //DirectX::XMFLOAT2 InvRenderTargetSize = {0.0f, 0.0f};
      float nearZ = 0.0f;
      float farZ = 0.0f;
      float totalTime = 0.0f;
      float deltaTime = 0.0f;
   };

   struct BasicObjectConstants
   {
      DirectX::SimpleMath::Matrix M;
   };

   struct BasicRenderItem
   {
      DirectX::SimpleMath::Matrix M = DirectX::SimpleMath::Matrix::Identity;

      UINT framesDirty = FRAME_RESOURCES_COUNT;

      // no object count because there is only 1 object

      MeshBuffer<BasicVertex, uint16_t> * meshBuffer = nullptr;

      D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

      UINT indexCount = 0;
      UINT startIndexLocation = 0;
      int baseVertexLocation = 0;
   };
}
#pragma once

#include "stdafx.h"
#include "CommittedDefaultBuffer.h"

namespace dmp
{
   struct VertexBindInfo
   {
      ID3D12GraphicsCommandList * commandList;
      size_t startSlot;
      // TODO: need to support multiple views?
   };

   template <typename VertexT>
   class CommittedVertexBuffer : public virtual CommittedDefaultBuffer<VertexT, VertexBindInfo>
   {
   public:
      virtual void bind(const VertexBindInfo & bi) override
      {
         D3D12_VERTEX_BUFFER_VIEW vbv;
         vbv.BufferLocation = mGPUBuffer->getGPUAddress();
         vbv.StrideInBytes = (UINT) alignment();
         vbv.SizeInBytes = ((UINT) alignment() * size());

         bi.commandList->IASetVertexBuffers(bi.startSlot, 1, &vbv);

         //TODO: residency
      }

      virtual void unbind(const VertexBindInfo & bi) override
      {
         // TODO: residency
      }

      ~CommittedVertexBuffer() {}
   };
}
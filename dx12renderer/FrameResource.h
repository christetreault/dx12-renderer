#pragma once

#include "UploadBuffer.h"

namespace dmp
{
   template <typename P, typename O>
   struct FrameResource
   {
      FrameResource(ID3D12Device * dev, size_t passCount, size_t objCount)
         : passCB(dev, passCount), objectCB(dev, objCount)
      {
         expectRes("Create Frame Resource Command Allocator",
                   dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(allocator
                                                            .ReleaseAndGetAddressOf())));
      }

      FrameResource(const FrameResource& rhs) = delete;
      FrameResource& operator=(const FrameResource& rhs) = delete;
      ~FrameResource() {}

      // fields
      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
      uint64_t fenceVal;
      
      ConstantUploadBuffer<P> passCB;
      ConstantUploadBuffer<O> objectCB;
   };
}

#pragma once
namespace dmp
{
   struct FrameResource
   {
      FrameResource(/* TODO: ID3D12Device * dev*/);
      FrameResource(const FrameResource& rhs) = delete;
      FrameResource& operator=(const FrameResource& rhs) = delete;
      ~FrameResource();
   };
}

#pragma once

#include "stdafx.h"

namespace dmp
{
   template <typename DataT>
   class DefaultBuffer
   {
   public:
      DefaultBuffer() = delete;
      DefaultBuffer(const DefaultBuffer &) = delete;
      DefaultBuffer & operator=(const DefaultBuffer &) = delete;

      DefaultBuffer(std::vector<DataT> & data,
                    ID3D12Device * dev,
                    ID3D12GraphicsCommandList * clist) :
         mByteSize(data.size() * sizeof(DataT)), mDataTypeSize(sizeof(DataT)), mData(data)
      {
         createDefaultBuffer(dev, clist);
      }

      D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const
      {
         return mGPUBuffer->GetGPUVirtualAddress();
      }

      UINT getByteSize() const { return (UINT) mByteSize; }
      UINT getDataTypeSize() const { return (UINT) mDataTypeSize; }

   private:
      void createDefaultBuffer(ID3D12Device * dev,
                               ID3D12GraphicsCommandList * clist)
      {
         expectRes("Create default buffer",
                   dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                                D3D12_HEAP_FLAG_NONE,
                                                &CD3DX12_RESOURCE_DESC::Buffer(mByteSize),
                                                D3D12_RESOURCE_STATE_COMMON,
                                                nullptr,
                                                IID_PPV_ARGS(mGPUBuffer.ReleaseAndGetAddressOf())));

         expectRes("Create upload buffer",
                   dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                D3D12_HEAP_FLAG_NONE,
                                                &CD3DX12_RESOURCE_DESC::Buffer(mByteSize),
                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                nullptr,
                                                IID_PPV_ARGS(mUploader.ReleaseAndGetAddressOf())));

         D3D12_SUBRESOURCE_DATA srd = {};
         srd.pData = mData.data();
         srd.RowPitch = mByteSize;
         srd.SlicePitch = srd.RowPitch;

         clist->ResourceBarrier(1,
                                &CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
                                                                      D3D12_RESOURCE_STATE_COMMON,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST));
         UpdateSubresources<1>(clist, mGPUBuffer.Get(), mUploader.Get(), 0, 0, 1, &srd);
         clist->ResourceBarrier(1,
                                &CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_GENERIC_READ));
      }

      size_t mByteSize = 0;
      size_t mDataTypeSize = 0;
      std::vector<DataT> mData;
      Microsoft::WRL::ComPtr<ID3D12Resource> mGPUBuffer = nullptr;
      Microsoft::WRL::ComPtr<ID3D12Resource> mUploader = nullptr;
   };
}
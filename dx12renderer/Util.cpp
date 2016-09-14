#include "stdafx.h"
#include "Util.h"
#include <fstream>

Microsoft::WRL::ComPtr<ID3DBlob> dmp::readShaderBinary(const std::string & path)
{
   std::ifstream fin(path, std::ios::binary);
   fin.seekg(0, std::ios_base::end);
   auto endPos = fin.tellg();
   fin.seekg(0, std::ios_base::beg);

   Microsoft::WRL::ComPtr<ID3DBlob> blob;
   expectRes("Create shader blob",
             D3DCreateBlob(endPos, blob.ReleaseAndGetAddressOf()));

   fin.read((char*) blob->GetBufferPointer(), endPos);
   fin.close();

   return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> dmp::createDefaultBuffer(ID3D12Device * dev, 
                                                                ID3D12GraphicsCommandList * clist, 
                                                                const void * data, 
                                                                size_t byteSize, 
                                                                Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuf)
{
   
   Microsoft::WRL::ComPtr<ID3D12Resource> retval;

   expectRes("Create default buffer",
             dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                          D3D12_HEAP_FLAG_NONE,
                                          &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                                          D3D12_RESOURCE_STATE_COMMON,
                                          nullptr,
                                          IID_PPV_ARGS(retval.ReleaseAndGetAddressOf())));

   expectRes("Create upload buffer",
             dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                          D3D12_HEAP_FLAG_NONE,
                                          &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr,
                                          IID_PPV_ARGS(uploadBuf.ReleaseAndGetAddressOf())));

   D3D12_SUBRESOURCE_DATA srd = {};
   srd.pData = data;
   srd.RowPitch = byteSize;
   srd.SlicePitch = srd.RowPitch;

   clist->ResourceBarrier(1,
                          &CD3DX12_RESOURCE_BARRIER::Transition(retval.Get(),
                                                                D3D12_RESOURCE_STATE_COMMON,
                                                                D3D12_RESOURCE_STATE_COPY_DEST));
   UpdateSubresources<1>(clist, retval.Get(), uploadBuf.Get(), 0, 0, 1, &srd);
   clist->ResourceBarrier(1,
                          &CD3DX12_RESOURCE_BARRIER::Transition(retval.Get(),
                                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                                D3D12_RESOURCE_STATE_GENERIC_READ));

   return retval;
}

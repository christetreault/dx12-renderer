#pragma once

#include "stdafx.h"

namespace dmp
{
   struct SubMesh
   {
      size_t indexCount = 0;
      size_t startIndexOffset = 0;
      size_t startVertexOffset = 0;
   };

   template <typename VertexT>
   class MeshData
   {
   public:
      MeshData(const std::vector<VertexT> & verts, std::string name = "") :
         mName(name), mVerts(verts), mIndexed(false) {}
      MeshData(const std::vector<VertexT> & verts, const std::vector<uint16_t> i16, std::string name = "") :
         mName(name), mVerts(verts), mI16(i16), mIndexed(true) {}
      MeshData(const std::vector<VertexT> & verts, const std::vector<uint32_t> i32, std::string name = "") :
         mName(name), mVerts(verts), mI32(i32), mIndexed(true) {}

      bool isIndexed() const
      {
         expectTrue("Non-indexed vertices are currently not supported",
                    mIndexed);
         return mIndexed;
      }

      const std::string & getName() const { return mName; }
      void setName(const std::string & name) { mName = name; }

      const std::vector<VertexT> & getVerts()
      {
         return mVerts;
      }

      const std::vector<uint16_t> & getI16()
      {
         expectTrue("vertex is indexed", mIndexed);

         if (mI16.empty())
         {
            for (const auto & curr : mI32)
            {
               mI16.push_back(static_cast<uint16_t>(curr));
            }
         }

         return mI16;
      }

      const std::vector<uint32_t> & getI32()
      {
         expectTrue("vertex is indexed", mIndexed);

         if (mI32.empty())
         {
            for (const auto & curr : mI16)
            {
               mI32.push_back(static_cast<uint32_t>(curr));
            }
         }

         return mI32;
      }
   private:
      std::string mName;
      bool mIndexed = false;
      std::vector<VertexT> mVerts;
      std::vector<uint16_t> mI16;
      std::vector<uint32_t> mI32;
   };

   template <typename VertexT, typename IndexT>
   class MeshBuffer
   {
   public:
      MeshBuffer() = default;

      MeshBuffer(const std::string & name,
                 std::vector<MeshData<VertexT>> verts,
                 ID3D12Device * dev,
                 ID3D12GraphicsCommandList * clist) :
         mName(name)
      {
         if (std::is_same<IndexT, uint16_t>::value)
         {
            mIdxFormat = DXGI_FORMAT_R16_UINT;
         }
         else if (std::is_same<IndexT, uint32_t>::value)
         {
            mIdxFormat = DXGI_FORMAT_R32_UINT;
         }
         else expectTrue("IndexT must be uint32_t or uint16_t!", false);

         init(verts, dev, clist);
      }

      ~MeshBuffer() {}

      const std::string & name() const { return mName; }
      
      D3D12_VERTEX_BUFFER_VIEW vertexBufferView() const
      {
         D3D12_VERTEX_BUFFER_VIEW vbv;
         vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
         vbv.StrideInBytes = sizeof(VertexT);
         vbv.SizeInBytes = (UINT)mVertexByteSize;

         return vbv;
      }

      D3D12_INDEX_BUFFER_VIEW indexBufferView() const
      {
         D3D12_INDEX_BUFFER_VIEW ibv;
         ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
         ibv.Format = mIdxFormat;
         ibv.SizeInBytes = (UINT)mIndexByteSize;

         return ibv;
      }

   private:
      void init(std::vector<MeshData<VertexT>> verts, 
                ID3D12Device * dev,
                ID3D12GraphicsCommandList * clist)
      {
         size_t indexOffset = 0;
         size_t vertexOffset = 0;

         for (MeshData<VertexT> & curr : verts)
         {
            if (curr.getName() == "") continue;

            SubMesh sm;
            sm.indexCount = curr.getI16().size();
            sm.startIndexOffset = indexOffset;
            sm.startVertexOffset = vertexOffset;
         
            mSubMeshes[curr.getName()] = sm;

            indexOffset = indexOffset + curr.getI16().size();
            vertexOffset = vertexOffset + curr.getVerts().size();
         }

         std::vector<VertexT> allVerts(0);
         
         std::vector<IndexT> allIdxs(0);
         

         for (MeshData<VertexT> & curr : verts)
         {
            allVerts.insert(allVerts.end(), curr.getVerts().begin(), curr.getVerts().end());
            if (std::is_same<IndexT, uint16_t>::value)
            {
               allIdxs.insert(allIdxs.end(), curr.getI16().begin(), curr.getI16().end());
            }
            else if (std::is_same<IndexT, uint32_t>::value)
            {
               allIdxs.insert(allIdxs.end(), curr.getI32().begin(), curr.getI32().end());
            }
         }

         mVertexByteSize = allVerts.size() * sizeof(VertexT);
         mIndexByteSize = allIdxs.size() * sizeof(IndexT);

         auto allVertsByteSize = allVerts.size() * sizeof(VertexT);
         expectRes("Create vertex blob",
                   D3DCreateBlob(allVertsByteSize,
                                 vertexBufferCPU.ReleaseAndGetAddressOf()));
         CopyMemory(vertexBufferCPU->GetBufferPointer(), allVerts.data(), allVertsByteSize);

         auto allIdxsByteSize = allIdxs.size() * sizeof(IndexT);
         expectRes("Create index blob",
                   D3DCreateBlob(allIdxsByteSize,
                                 indexBufferCPU.ReleaseAndGetAddressOf()));
         CopyMemory(indexBufferCPU->GetBufferPointer(), allIdxs.data(), allIdxsByteSize);

         vertexBufferGPU = createDefaultBuffer(dev, clist, allVerts.data(), allVertsByteSize, vertexBufferUploader);
         indexBufferGPU = createDefaultBuffer(dev, clist, allIdxs.data(), allIdxsByteSize, indexBufferUploader);
      }

      bool mValid = false;
      std::string mName;

      Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
      Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;

      Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
      Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

      Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
      Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

      DXGI_FORMAT mIdxFormat;

      size_t mVertexByteSize;
      size_t mIndexByteSize;

      public:
         std::unordered_map<std::string, SubMesh> mSubMeshes;
   };
}
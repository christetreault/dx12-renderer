#pragma once



namespace dmp
{
   template <typename VertexT> class MeshData;
   struct BasicVertex;

   std::vector<MeshData<BasicVertex>> loadModel(const std::string & filename);
}
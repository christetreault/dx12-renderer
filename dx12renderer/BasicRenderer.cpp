#include "stdafx.h"
#include "BasicRenderer.h"
#include "BaseRenderer.h"
#include "UploadBuffer.h"
#include "Mesh.h"
#include "BasicModel.h"

dmp::BasicRenderer::BasicRenderer()
{}


dmp::BasicRenderer::~BasicRenderer()
{}

HRESULT dmp::BasicRenderer::drawPre()
{
   using namespace DirectX;
   auto allocator = mCurrFrameResource->allocator;

   expectRes("Reset the frame resource's allocator",
             allocator->Reset());

   // hardcoding PSO for now
   expectRes("Reset the command list",
             mCommandList->Reset(allocator.Get(), mPSOs[opaqueKey].Get()));

   mCommandList->RSSetViewports(1, &mViewport);
   mCommandList->RSSetScissorRects(1, &mScissorRect);

   mCommandList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(),
                                                                       D3D12_RESOURCE_STATE_PRESENT,
                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET));

   XMVECTORF32 clearColor = {mClearColor.x, mClearColor.y, mClearColor.z, mClearColor.w};

   mCommandList->ClearRenderTargetView(currentBackBufferView(),
                                       clearColor,
                                       0,
                                       nullptr);

   mCommandList->ClearDepthStencilView(depthStencilView(),
                                       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       1.0f, 0, 0, nullptr);

   mCommandList->OMSetRenderTargets(1, &currentBackBufferView(),
                                    true,
                                    &depthStencilView());

   ID3D12DescriptorHeap * dh[] = {mCBVHeap.Get()};
   mCommandList->SetDescriptorHeaps(_countof(dh), dh);

   mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

   int passCBVIndex = mPassCBVOffset + mCurrFrameResourceIndex;
   auto passCBVHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCBVHeap->GetGPUDescriptorHandleForHeapStart());

   passCBVHandle.Offset(passCBVIndex, mCbvSrvUavDescriptorSize);
   mCommandList->SetGraphicsRootDescriptorTable(1, passCBVHandle);

   return S_OK;
}

HRESULT dmp::BasicRenderer::drawImpl()
{
   UINT objSize = (UINT)calcConstantBufferByteSize(sizeof(BasicObjectConstants));

   auto clist = mCommandList.Get();

   for (auto & curr : mVger)
   {
      clist->IASetVertexBuffers(0, 1, &curr->meshBuffer->vertexBufferView());
      clist->IASetIndexBuffer(&curr->meshBuffer->indexBufferView());
      clist->IASetPrimitiveTopology(curr->primitiveType);

      UINT cbvIndex = mCurrFrameResourceIndex;
      auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCBVHeap->GetGPUDescriptorHandleForHeapStart());
      handle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

      clist->SetGraphicsRootDescriptorTable(0, handle);
      clist->DrawIndexedInstanced(curr->indexCount,
                                  1,
                                  curr->startIndexLocation,
                                  curr->baseVertexLocation,
                                  0);
   }

   return S_OK;
}

HRESULT dmp::BasicRenderer::drawPost()
{
   mCommandList->ResourceBarrier(1,
                                 &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer(),
                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                       D3D12_RESOURCE_STATE_PRESENT));

   expectRes("Close the command list, done with this pass",
             mCommandList->Close());

   ID3D12CommandList * clist[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(clist), clist);

   expectRes("Swap buffers",
             mSwapChain->Present(0, 0));
   mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

   mCurrFrameResource->fenceVal = ++mFenceVal;
   mCommandQueue->Signal(mFence.Get(), mFenceVal);

   return S_OK;
}

bool dmp::BasicRenderer::initImpl()
{
   using namespace DirectX::SimpleMath;
   expectRes("Reset Command List",
             mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

   expectTrue("Build root signature", buildRootSignature());
   expectTrue("Load Shaders and build Input Layouts", buildShadersAndInputLayouts());
   expectTrue("Build mesh buffer and upload", buildMeshBuffer());
   expectTrue("Build render item", buildRenderItems());
   expectTrue("Build frame resources", buildFrameResources());
   expectTrue("Build CBV descriptor heap", buildCBVDescriptorHeaps());
   expectTrue("Build Constant Buffer Views", buildConstantBufferViews());
   expectTrue("Build PSOs", buildPSOs());
   
   Vector3 eye(0.0f, 0.0f, 5.0f);
   Vector3 target(0.0f, 0.0f, 0.0f);
   Vector3 up(0.0f, 1.0f, 0.0f);

   mV = Matrix::CreateLookAt(eye, target, up);

   // actuall do all of that stuff ^
   expectRes("Close the command list", mCommandList->Close());
   ID3D12CommandList * lists[] = {mCommandList.Get()};
   mCommandQueue->ExecuteCommandLists(_countof(lists), lists);

   flushCommandQueue();

   return true;
}

bool dmp::BasicRenderer::buildRootSignature()
{
   using Microsoft::WRL::ComPtr;

   CD3DX12_DESCRIPTOR_RANGE objectCB;
   objectCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

   CD3DX12_DESCRIPTOR_RANGE passCB;
   passCB.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

   CD3DX12_ROOT_PARAMETER params[2];

   params[0].InitAsDescriptorTable(1, &objectCB);
   params[1].InitAsDescriptorTable(1, &passCB);

   // TODO: investigate CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC that is not defined in your copy of
   // cd3dx12.h
   CD3DX12_ROOT_SIGNATURE_DESC rsd(2,
                                   params,
                                   0,
                                   nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

   ComPtr<ID3DBlob> srs = nullptr;
   ComPtr<ID3DBlob> error = nullptr;

   //TODO: "This function has been superceded by D3D12SerializeVersionedRootSignature."

   auto res = D3D12SerializeRootSignature(&rsd,
                                          D3D_ROOT_SIGNATURE_VERSION_1,
                                          srs.GetAddressOf(),
                                          error.GetAddressOf());

   if (error != nullptr)
   {
      ::OutputDebugStringA((char*) error->GetBufferPointer());
   }

   expectRes("Serialize root signature", res);

   expectRes("Create Root signature",
             mDevice->CreateRootSignature(0,
                                          srs->GetBufferPointer(),
                                          srs->GetBufferSize(),
                                          IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::buildShadersAndInputLayouts()
{
   mShaders[vertexShaderFile] = readShaderBinary(vertexShaderFile);
   mShaders[pixelShaderFile] = readShaderBinary(pixelShaderFile);

   mInputLayout =
   {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      offsetof(BasicVertex, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
      offsetof(BasicVertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
   };

   return true;
}

bool dmp::BasicRenderer::buildMeshBuffer()
{
   //std::vector<BasicVertex> verts = {
   //   {DirectX::XMFLOAT4(-0.8f, -0.8f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
   //   {DirectX::XMFLOAT4(-0.8f, 0.8f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
   //   {DirectX::XMFLOAT4(0.8f, 0.8f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
   //   {DirectX::XMFLOAT4(0.8f, -0.8f, 0.0f, 1.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)}
   //};
   //std::vector<uint16_t> idxs = {0, 1, 2, 2, 3, 0};
   //MeshData<BasicVertex> md(verts, idxs, "helloSayer");
   
   // TODO: barf; magic constants
   auto models = loadModel("Voyager.lwo");

   

   //auto vger = std::accumulate(models.begin(),
   //                              models.end(),
   //                              MeshData<BasicVertex>(std::vector<BasicVertex>(), std::vector<uint16_t>(), "vger"),
   //                              [](MeshData<BasicVertex> acc, MeshData<BasicVertex> x)
   //{
   //   auto offset = (uint16_t)acc.getVerts().size();
   //   auto accV = acc.getVerts();
   //   auto accI = acc.getI16();
   //   auto name = acc.getName();

   //   auto xV = x.getVerts();
   //   auto xI = x.getI16();

   //   for (auto & curr : xI) curr = curr + offset;

   //   for (const auto & curr : xI) accI.push_back(curr);
   //   for (const auto & curr : xV) accV.push_back(curr);

   //   return MeshData<BasicVertex>(accV, accI, name);
   //});

   for (size_t i = 0; i < models.size(); ++i)
   {
      std::string name = "vger " + std::to_string(i);
      models[i].setName(name);
   }

   std::vector<MeshData<BasicVertex>> mds = {};

   mMeshBuffer = std::make_unique<MeshBuffer<BasicVertex, uint16_t>>("Guardian of the hellos", models, mDevice.Get(), mCommandList.Get());

   return true;
}

bool dmp::BasicRenderer::buildRenderItems()
{
   for (auto & curr : mMeshBuffer->mSubMeshes)
   {
      auto vger = std::make_unique<BasicRenderItem>();
      auto submesh = curr.second;
      vger->meshBuffer = mMeshBuffer.get();
      vger->indexCount = (UINT) submesh.indexCount;
      vger->startIndexLocation = (UINT) submesh.startIndexOffset;
      vger->baseVertexLocation = (UINT) submesh.startVertexOffset;
      
      //mVger[0]->meshBuffer = mMeshBuffer.get();
      //mVger[0]->indexCount = (UINT) mVger[0]->meshBuffer->mSubMeshes["vger 0"].indexCount;
      //mVger[0]->startIndexLocation = (UINT) mVger[0]->meshBuffer->mSubMeshes["vger 0"].startIndexOffset;
      //mVger[0]->baseVertexLocation = (UINT) mVger[0]->meshBuffer->mSubMeshes["vger 0"].startVertexOffset;

      mVger.push_back(std::move(vger));
   }

   return true;
}

bool dmp::BasicRenderer::buildFrameResources()
{
   for (size_t i = 0; i < FRAME_RESOURCES_COUNT; ++i)
   {
      mFrameResources.push_back(std::make_unique<FrameResource<BasicPassConstants, BasicObjectConstants>>(mDevice.Get(), 1, 1));
   }

   return true;
}

bool dmp::BasicRenderer::buildCBVDescriptorHeaps()
{
   UINT objCount = 1; // Just the triangle

   // one descriptor per object for object constants
   // 1 extra descriptor for pass constants
   // times 3 since we need this per frame resource
   UINT descCount = (objCount + 1) * FRAME_RESOURCES_COUNT;

   mPassCBVOffset = objCount * FRAME_RESOURCES_COUNT;

   D3D12_DESCRIPTOR_HEAP_DESC dhd;
   dhd.NumDescriptors = descCount;
   dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   dhd.NodeMask = 0;

   expectRes("Create CBV descriptor heaps",
             mDevice->CreateDescriptorHeap(&dhd,
                                           IID_PPV_ARGS(mCBVHeap.ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::buildConstantBufferViews()
{
   UINT objCBSize = (UINT)calcConstantBufferByteSize(sizeof(BasicObjectConstants));

   UINT objCount = 1;
   
   for (size_t frameIndex = 0; frameIndex < FRAME_RESOURCES_COUNT; ++frameIndex)
   {
      auto objCB = mFrameResources[frameIndex]->objectCB->getBuffer();

      // will only iterate once, but here for "documentation" purposes
      // a _sufficiently_smart_compiler_ should optimize it out after all
      
      for (UINT i = 0; i < objCount; ++i)
      {
         D3D12_GPU_VIRTUAL_ADDRESS cbAddr = objCB->GetGPUVirtualAddress();

         // offset to the ith object constant buffer
         cbAddr = cbAddr + (i * objCBSize);

         // offset to the object cbv in the descriptor heap
         int heapIndex = (int)((frameIndex * objCount) + i);
         expectTrue("mCBVHeap not null", mCBVHeap);
         auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
         handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

         D3D12_CONSTANT_BUFFER_VIEW_DESC cvd;
         cvd.BufferLocation = cbAddr;
         cvd.SizeInBytes = (UINT)objCBSize;

         mDevice->CreateConstantBufferView(&cvd, handle);
      }
   }

   auto passCBSize = calcConstantBufferByteSize(sizeof(BasicPassConstants));

   for (int frameIndex = 0; frameIndex < FRAME_RESOURCES_COUNT; ++frameIndex)
   {
      auto passCB = mFrameResources[frameIndex]->passCB->getBuffer();
      D3D12_GPU_VIRTUAL_ADDRESS cbAddr = passCB->GetGPUVirtualAddress();

      int heapIndex = mPassCBVOffset + frameIndex;
      // if we made it here, then this must have passed the previous null check
      auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
      handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

      D3D12_CONSTANT_BUFFER_VIEW_DESC cvd;
      cvd.BufferLocation = cbAddr;
      cvd.SizeInBytes = (UINT)passCBSize;

      mDevice->CreateConstantBufferView(&cvd, handle);

   }

   return true;
}

bool dmp::BasicRenderer::buildPSOs()
{
   D3D12_GRAPHICS_PIPELINE_STATE_DESC psd = {};
   psd.InputLayout = {mInputLayout.data(), (UINT) mInputLayout.size()};
   psd.pRootSignature = mRootSignature.Get();
   psd.VS =
   {
      reinterpret_cast<BYTE*>(mShaders[vertexShaderFile]->GetBufferPointer()),
      mShaders[vertexShaderFile]->GetBufferSize()
   };
   psd.PS =
   {
      reinterpret_cast<BYTE*>(mShaders[pixelShaderFile]->GetBufferPointer()),
      mShaders[pixelShaderFile]->GetBufferSize()
   };

   psd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   psd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
   psd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   psd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
   psd.SampleMask = UINT_MAX; // don't feel like figuring out exactly what this is in std::limits
   psd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   psd.NumRenderTargets = 1;
   psd.RTVFormats[0] = mBackBufferFormat;
   psd.SampleDesc.Count = mMsaaEnabled ? 4 : 1;
   psd.SampleDesc.Quality = mMsaaEnabled ? (mMsaaQualityLevel - 1) : 0;
   psd.DSVFormat = mDepthStencilFormat;

   expectRes("Create opaque PSO",
             mDevice->CreateGraphicsPipelineState(&psd,
                                                  IID_PPV_ARGS(mPSOs[opaqueKey].ReleaseAndGetAddressOf())));

   // might as well make a wireframe PSO too

   auto wfPsd = psd;
   wfPsd.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
   expectRes("Create wireframe PSO",
             mDevice->CreateGraphicsPipelineState(&wfPsd,
                                                  IID_PPV_ARGS(mPSOs[wireframeKey].ReleaseAndGetAddressOf())));

   return true;
}

bool dmp::BasicRenderer::updatePre(const Timer & t) // TODO: need a busy callback
{
   mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % FRAME_RESOURCES_COUNT;
   mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

   if (mCurrFrameResource->fenceVal != 0 && mFence->GetCompletedValue() < mCurrFrameResource->fenceVal)
   {
      auto handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
      expectRes("Set event to allow GPU to catch up to CPU",
                mFence->SetEventOnCompletion(mCurrFrameResource->fenceVal, handle));
      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
   }

   return true;
}

bool dmp::BasicRenderer::updateImpl(const Timer & t)
{
   using namespace DirectX;
   using namespace DirectX::SimpleMath;

   // Clear color

   auto time = t.time();

   float lerpColor = std::fmod(time * 0.08f, 1.0f);
   float theta = std::fmod(time /** 0.08f*/, XM_2PI);
   float rotTheta = std::fmod(time, XM_2PI);

   rotTheta = rotTheta - (XM_PI * 0.5f);
   rotTheta = rotTheta > (XM_PI * 0.5f) ? XM_PI - rotTheta : rotTheta;

   XMVECTOR c1;
   XMVECTOR c2;

   if (lerpColor < 0.33f)
   {
      c1 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
      c2 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, lerpColor * 3.0f));
   }
   else if (lerpColor < 0.66f)
   {
      c1 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
      c2 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, (lerpColor - 0.33f) * 3.0f));
   }
   else
   {
      c1 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
      c2 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
      XMStoreFloat4(&mClearColor, XMVectorLerp(c1, c2, (lerpColor - 0.66f) * 3.0f));
   }
   
   // Pass Constants

   BasicPassConstants pc;

   pc.V = mV.Transpose();
   pc.inverseV = mV.Invert().Transpose();
   pc.P = mP.Transpose();
   pc.inverseP = mP.Invert().Transpose();
   pc.PV = (mV * mP).Transpose();
   pc.inversePV = (mV * mP).Invert().Transpose();
   pc.eye = Vector4(0.0f, 0.0f, 5.0f, 1.0f);
   pc.nearZ = 1.0f;
   pc.farZ = 1000.0f;
   pc.totalTime = t.time();
   pc.deltaTime = t.deltaTime();

   mCurrFrameResource->passCB->copyData(0, pc);

   for (auto & curr : mVger)
   {
      curr->M = Matrix::CreateRotationY(theta)
         * Matrix::CreateRotationZ(-theta)
         * Matrix::CreateRotationX(theta)
         * Matrix::CreateTranslation(Vector3(sin(theta), cos(theta), 0.0f));

      curr->framesDirty = FRAME_RESOURCES_COUNT;

      auto & objCB = mCurrFrameResource->objectCB;

      if (curr->framesDirty > 0) // which it will be
      {
         // TODO: XMStoreFloat4x4 didn't invalidate M did it? That would suck...

         BasicObjectConstants oc;
         oc.M = curr->M.Transpose();

         objCB->copyData(0, oc);

         curr->framesDirty--;
      }
   }

   return true;
}

HRESULT dmp::BasicRenderer::resizeImpl(int width, int height, bool force)
{
   using namespace DirectX::SimpleMath;
   BaseRenderer::resizeImpl(width, height, force);

   if (!force) return S_OK;

   mP = Matrix::CreatePerspectiveFieldOfView(0.25 * DirectX::XM_PI, getAspectRatio(), 1.0f, 1000.0f);

   return S_OK;
}
#include "Renderer/PathTracing.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FPathTracingPass::FPathTracingPass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t InWidth, uint32_t InHeight)
    : GraphicsDevice(Device),
    PathTracingPassPipelineState(Device->GetDevice(), FRaytracingPipelineStateCreationDesc{
      .rtShaderPath = L"Shaders/Raytracing/PathTracing.hlsl",
      .EntryPointRGS = L"RayGen",
      .EntryPointCHS = L"ClosestHit",
      .EntryPointRMS = L"Miss",
	}),
    PathTracingPassSBT(Device, PathTracingPassPipelineState.GetRTStateObjectProps())
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FPathTracingPass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FPathTracingPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;

    FTextureCreationDesc PathTracingSceneTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"PathTracing SceneTexture",
    };

    FTextureCreationDesc FrameAccumulatedDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16_UINT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"PathTracing FrameAccumulated",
    };

    PathTracingSceneTexture = Device->CreateTexture(PathTracingSceneTextureDesc);
    FrameAccumulatedTexture = Device->CreateTexture(FrameAccumulatedDesc);
}

void FPathTracingPass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    GraphicsContext->AddResourceBarrier(PathTracingSceneTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDesc = PathTracingPassSBT.CreateRayDesc(Width, Height);

    // Bind the raytracing pipeline
    GraphicsContext->SetRaytracingPipelineState(PathTracingPassPipelineState);
    GraphicsContext->SetRaytracingComputeRootSignature();

    // Bind
    GraphicsContext->SetComputeRootShaderResourceView(
        RTParams_SceneDescriptor, Scene->GetRaytracingScene().GetTopLevelASGPUVirtualAddress()); // t0

    bool IsViewProjectChanged = Scene->GetCamera().IsViewProjMatrixChanged();

    interlop::PathTraceRenderResources RenderResources = {
        .invViewProjectionMatrix = Scene->GetCamera().GetInvViewProjMatrix(),
        .dstTextureIndex = PathTracingSceneTexture.UavIndex,
		.frameAccumulatedTextureIndex = FrameAccumulatedTexture.UavIndex,
        .geometryInfoBufferIdx = Scene->GetRaytracingScene().GetGeometryInfoBufferSrv(),
        .materialBufferIdx = Scene->GetRaytracingScene().GetMaterialBufferSrv(),
        .vtxBufferIdx = Scene->GetRaytracingScene().GetMeshVertexBufferSrv(),
        .idxBufferIdx = Scene->GetRaytracingScene().GetIndiceBufferSrv(),
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex,
        .envmapTextureIndex = Scene->GetEnvironmentMap()->CubeMapTexture.SrvIndex,
		.envmapIntensity = Scene->EnvmapIntensity,
        .maxPathDepth = 10,
        .numSamples = (uint32_t)Scene->PathTracingSamplePerPixel,
		.bRefreshPathTracingTexture = IsViewProjectChanged ? 1u : 0u,
    };

    for (int i = 0; i < 16; i++)
    {
		RenderResources.randomFloats[i] = XMFLOAT4{
			static_cast<float>(rand()) / RAND_MAX,
			static_cast<float>(rand()) / RAND_MAX,
			static_cast<float>(rand()) / RAND_MAX,
			static_cast<float>(rand()) / RAND_MAX
		};
    }

    GraphicsContext->SetComputeRoot32BitConstants(RTParams_CBuffer, 48u, &RenderResources);

    // Dispatch the rays and write to the raytracing output
    GraphicsContext->DispatchRays(RayDesc);

    // reset default rootSignature
    GraphicsContext->SetComputeRootSignature();
}

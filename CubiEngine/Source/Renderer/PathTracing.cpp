#include "Renderer/PathTracing.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FPathTracingPass::FPathTracingPass(uint32_t InWidth, uint32_t InHeight)
    : FRenderPass(InWidth, InHeight),
    PathTracingPassPipelineState(FRaytracingPipelineStateCreationDesc{
      .rtShaderPath = L"Shaders/Raytracing/PathTracing.hlsl",
      .EntryPointRGS = L"RayGen",
      .EntryPointCHS = L"ClosestHit",
      .EntryPointRMS = L"Miss",
      .EntryPointAHS = L"AnyHit",
	}),
    PathTracingPassSBT(PathTracingPassPipelineState.GetRTStateObjectProps())
{
}

void FPathTracingPass::InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight)
{
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

    FTextureCreationDesc PathTracingAlbedoTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"PathTracing Albedo",
    };

    FTextureCreationDesc PathTracingNormalTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"PathTracing Normal",
    };

    PathTracingSceneTexture = RHICreateTexture(PathTracingSceneTextureDesc);
    FrameAccumulatedTexture = RHICreateTexture(FrameAccumulatedDesc);

    PathTracingAlbedoTexture = RHICreateTexture(PathTracingAlbedoTextureDesc);
    PathTracingNormalTexture = RHICreateTexture(PathTracingNormalTextureDesc);
}

void FPathTracingPass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    GraphicsContext->AddResourceBarrier(PathTracingSceneTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->AddResourceBarrier(PathTracingAlbedoTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->AddResourceBarrier(PathTracingNormalTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
        .dstTextureIndex = PathTracingSceneTexture->UavIndex,
        .albedoTextureIndex = PathTracingAlbedoTexture->UavIndex,
        .normalTextureIndex = PathTracingNormalTexture->UavIndex,
		.frameAccumulatedTextureIndex = FrameAccumulatedTexture->UavIndex,
        .geometryInfoBufferIdx = Scene->GetRaytracingScene().GetGeometryInfoBufferSrv(),
        .materialBufferIdx = Scene->GetRaytracingScene().GetMaterialBufferSrv(),
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex,
        .envmapTextureIndex = Scene->GetEnvironmentMap()->CubeMapTexture->SrvIndex,
		.envmapIntensity = Scene->GetRenderSettings().EnvmapIntensity,
        .envBRDFTextureIndex = Scene->GetEnvironmentMap()->BRDFLutTexture->SrvIndex,
        .debugBufferIndex = Scene->GetDebugBuffer().CbvIndex,
        .maxPathDepth = 10,
        .numSamples = (uint32_t)Scene->GetRenderSettings().PathTracingSamplePerPixel,
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

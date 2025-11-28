#include "Renderer/RaytracingShadowPass.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FRaytracingShadowPass::FRaytracingShadowPass(uint32_t InWidth, uint32_t InHeight)
	: FRenderPass(InWidth, InHeight),
    RaytracingShadowPipelineState(FRaytracingPipelineStateCreationDesc{
      .rtShaderPath = L"Shaders/Raytracing/RaytracingShadow.hlsl",
      .EntryPointRGS = L"RayGen",
      .EntryPointCHS = L"ClosestHit",
      .EntryPointRMS = L"Miss",
    }),
    RaytracingShadowPassSBT(RaytracingShadowPipelineState.GetRTStateObjectProps())
{
}

void FRaytracingShadowPass::InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc RaytracingShadowTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"RaytracingShadow Texture",
    };

    RaytracingShadowTexture = RHICreateTexture(RaytracingShadowTextureDesc);
}

void FRaytracingShadowPass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(RaytracingShadowTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDesc = RaytracingShadowPassSBT.CreateRayDesc(Width, Height);

    // Bind the raytracing pipeline
    GraphicsContext->SetRaytracingPipelineState(RaytracingShadowPipelineState);
    GraphicsContext->SetRaytracingComputeRootSignature();

    // Bind
    GraphicsContext->SetComputeRootShaderResourceView(
        RTParams_SceneDescriptor, Scene->GetRaytracingScene().GetTopLevelASGPUVirtualAddress()); // t0

    interlop::RaytracingShadowRenderResource RenderResources = {
        .invViewProjectionMatrix = Scene->GetCamera().GetInvViewProjMatrix(),
        .dstTextureIndex = RaytracingShadowTexture->UavIndex,
        .depthTextureIndex = SceneTexture.DepthTexture->SrvIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex
    };

    GraphicsContext->SetComputeRoot32BitConstants(RTParams_CBuffer, 48u, &RenderResources);

    // Dispatch the rays and write to the raytracing output
    GraphicsContext->DispatchRays(RayDesc);

    // reset default rootSignature
    GraphicsContext->SetComputeRootSignature();
}

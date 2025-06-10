#include "Renderer/RaytracingShadowPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FRaytracingShadowPass::FRaytracingShadowPass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t InWidth, uint32_t InHeight)
    : GraphicsDevice(Device),
    RaytracingShadowPipelineState(Device->GetDevice(), FRaytracingPipelineStateCreationDesc{
      .rtShaderPath = L"Shaders/Raytracing/RaytracingShadow.hlsl",
      .EntryPointRGS = L"RayGen",
      .EntryPointCHS = L"ClosestHit",
      .EntryPointRMS = L"Miss",
    }),
    RaytracingShadowPassSBT(Device, RaytracingShadowPipelineState.GetRTStateObjectProps())
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FRaytracingShadowPass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FRaytracingShadowPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;

    FTextureCreationDesc RaytracingShadowTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"RaytracingShadow Texture",
    };

    RaytracingShadowTexture = Device->CreateTexture(RaytracingShadowTextureDesc);
}

void FRaytracingShadowPass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(RaytracingShadowTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
        .dstTextureIndex = RaytracingShadowTexture.UavIndex,
        .depthTextureIndex = SceneTexture.DepthTexture.SrvIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex
    };

    GraphicsContext->SetComputeRoot32BitConstants(RTParams_CBuffer, 48u, &RenderResources);

    // Dispatch the rays and write to the raytracing output
    GraphicsContext->DispatchRays(RayDesc);

    // reset default rootSignature
    GraphicsContext->SetComputeRootSignature();
}

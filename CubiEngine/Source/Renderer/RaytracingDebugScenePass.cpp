#include "Renderer/RaytracingDebugScenePass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FRaytracingDebugScenePass::FRaytracingDebugScenePass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t InWidth, uint32_t InHeight)
    : GraphicsDevice(Device),
      RaytracingDebugScenePassPipelineState(Device->GetDevice(), FRaytracingPipelineStateCreationDesc{
        .rtShaderPath = L"Shaders/Raytracing/DebugScene.hlsl",
        .EntryPointRGS = L"RayGen",
        .EntryPointCHS = L"ClosestHit",
        .EntryPointRMS = L"Miss",
        }),
      RaytracingDebugScenePassSBT(Device, RaytracingDebugScenePassPipelineState.GetRTStateObjectProps())
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FRaytracingDebugScenePass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FRaytracingDebugScenePass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;

    FTextureCreationDesc RaytracingDebugSceneTextureDesc = {
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"RaytracingDebugScene Texture",
    };
    RaytracingDebugSceneTexture = Device->CreateTexture(RaytracingDebugSceneTextureDesc);
}

void FRaytracingDebugScenePass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    GraphicsContext->AddResourceBarrier(RaytracingDebugSceneTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC RayDesc = RaytracingDebugScenePassSBT.CreateRayDesc(Width, Height);

    // Bind the raytracing pipeline
    GraphicsContext->SetRaytracingPipelineState(RaytracingDebugScenePassPipelineState);
    GraphicsContext->SetRaytracingComputeRootSignature();

    // Bind
    GraphicsContext->SetComputeRootShaderResourceView(
        RTParams_SceneDescriptor, Scene->GetRaytracingScene().GetTopLevelASGPUVirtualAddress()); // t0

    interlop::RTSceneDebugRenderResource RenderResources = {
        .invViewProjectionMatrix = Scene->GetCamera().GetInvViewProjMatrix(),
        .dstTextureIndex = RaytracingDebugSceneTexture.UavIndex,
        .geometryInfoBufferIdx = Scene->GetRaytracingScene().GetGeometryInfoBufferSrv(),
        .materialBufferIdx = Scene->GetRaytracingScene().GetMaterialBufferSrv(),
        .vtxBufferIdx = Scene->GetRaytracingScene().GetMeshVertexBufferSrv(),
        .idxBufferIdx = Scene->GetRaytracingScene().GetIndiceBufferSrv(),
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex,
        .envmapTextureIndex = Scene->GetEnvironmentMap()->CubeMapTexture.SrvIndex,
    };

    GraphicsContext->SetComputeRoot32BitConstants(RTParams_CBuffer, 32u, &RenderResources);

    // Dispatch the rays and write to the raytracing output
    GraphicsContext->DispatchRays(RayDesc);

    // reset default rootSignature
    GraphicsContext->SetComputeRootSignature();
}

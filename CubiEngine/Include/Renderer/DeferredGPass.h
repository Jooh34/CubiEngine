#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"
#include "Graphics/GraphicsDevice.h"

class FScene;
class FShadowDepthPass;

struct FGBuffer
{
    FTexture GBufferA{}; // Albedo
    FTexture GBufferB{}; // Normal
    FTexture GBufferC{}; // AO + MetalRoughness
    FTexture VelocityTexture;
};

class FDeferredGPass
{
public:
    FDeferredGPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        FSceneTexture& SceneTexture);

    void RenderLightPass(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        FShadowDepthPass* ShadowDepthPass, FSceneTexture& SceneTexture, FTexture* SSAOTexture, FTexture* RaytracingShadowTexture);

private:
    FPipelineState GeometryPassPipelineState;
    FPipelineState GeometryPassLightPipelineState;

    FPipelineState LightPassPipelineState;

};
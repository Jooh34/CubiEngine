#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingShadowPass : public FRenderPass
{
public:
    FRaytracingShadowPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);

    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);

    FTexture& GetRaytracingShadowTexture() { return RaytracingShadowTexture; }

private:
    const FGraphicsDevice* GraphicsDevice;

    FRaytracingPipelineState RaytracingShadowPipelineState;
    FShaderBindingTable RaytracingShadowPassSBT;

    FTexture RaytracingShadowTexture;
};
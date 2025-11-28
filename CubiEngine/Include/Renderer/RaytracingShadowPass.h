#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingShadowPass : public FRenderPass
{
public:
    FRaytracingShadowPass(uint32_t Width, uint32_t Height);

    void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) override;

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);

    FTexture* GetRaytracingShadowTexture() { return RaytracingShadowTexture.get(); }

private:
    FRaytracingPipelineState RaytracingShadowPipelineState;
    FShaderBindingTable RaytracingShadowPassSBT;

    std::unique_ptr<FTexture> RaytracingShadowTexture;
};
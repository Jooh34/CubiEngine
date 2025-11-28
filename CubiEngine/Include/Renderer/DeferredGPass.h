#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FScene;
class FShadowDepthPass;
class FGraphicsContext;

class FDeferredGPass : public FRenderPass
{
public:
    FDeferredGPass(uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) override;

    void Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        FSceneTexture& SceneTexture);

    void RenderLightPass(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        FShadowDepthPass* ShadowDepthPass, FSceneTexture& SceneTexture, FTexture* SSAOTexture, FTexture* RaytracingShadowTexture);

private:
    FPipelineState GeometryPassPipelineState;
    FPipelineState GeometryPassLightPipelineState;

    FPipelineState LightPassPipelineState;

};
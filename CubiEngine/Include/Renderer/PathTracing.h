#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FPathTracingPass : public FRenderPass
{
public:
    FPathTracingPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture* GetPathTracingSceneTexture() { return PathTracingSceneTexture.get(); }

private:
    FRaytracingPipelineState PathTracingPassPipelineState;
    FShaderBindingTable PathTracingPassSBT;

    std::unique_ptr<FTexture> PathTracingSceneTexture;
    std::unique_ptr<FTexture> FrameAccumulatedTexture;

    uint32_t CollectedFrame = 0;
    uint32_t SamplePerPixel = 8;
};

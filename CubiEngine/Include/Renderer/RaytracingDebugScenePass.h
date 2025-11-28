#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingDebugScenePass : public FRenderPass
{
public:
    FRaytracingDebugScenePass(uint32_t Width, uint32_t Height);

    void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) override;

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture* GetRaytracingDebugSceneTexture() { return RaytracingDebugSceneTexture.get(); }

private:
    FRaytracingPipelineState RaytracingDebugScenePassPipelineState;
    FShaderBindingTable RaytracingDebugScenePassSBT;

    std::unique_ptr<FTexture> RaytracingDebugSceneTexture;
};
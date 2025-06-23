#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingDebugScenePass : public FRenderPass
{
public:
    FRaytracingDebugScenePass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);

    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture& GetRaytracingDebugSceneTexture() { return RaytracingDebugSceneTexture; }

private:
    FRaytracingPipelineState RaytracingDebugScenePassPipelineState;
    FShaderBindingTable RaytracingDebugScenePassSBT;

    FTexture RaytracingDebugSceneTexture;
};
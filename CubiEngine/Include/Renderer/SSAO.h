#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FSSAOPass : public FRenderPass
{
public:
    FSSAOPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;
    
    void GenerateSSAOKernel();
    void AddSSAOPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    
    std::unique_ptr<FTexture> SSAOTexture;

private:
    FPipelineState SSAOPipelineState;

    FBuffer SSAOKernelBuffer;
};
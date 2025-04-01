#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FSSAO
{
public:
    FSSAO(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    
    void GenerateSSAOKernel();
    void AddSSAOPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    
    FTexture SSAOTexture;

private:
    FPipelineState SSAOPipelineState;

    FBuffer SSAOKernelBuffer;
};
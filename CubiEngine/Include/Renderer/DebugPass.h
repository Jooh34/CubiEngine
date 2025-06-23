#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FDebugPass : public FRenderPass
{
public:
    FDebugPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void Copy(FGraphicsContext* const GraphicsContext, FTexture& SrcTexture, FTexture& DstTexture, uint32_t Width, uint32_t Height);

    FPipelineState CopyPipelineState;

    FTexture TextureForCopy;
};

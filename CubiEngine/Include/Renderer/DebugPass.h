#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FDebugPass
{
public:
    FDebugPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void Copy(FGraphicsContext* const GraphicsContext, const FTexture& SrcTexture, const FTexture& DstTexture, uint32_t Width, uint32_t Height);

    FPipelineState CopyPipelineState;

    FTexture TextureForCopy;
};

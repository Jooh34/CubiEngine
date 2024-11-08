#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FPostProcess
{
public:
    FPostProcess(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void Tonemapping(FGraphicsContext* const GraphicsContext,
        FTexture& SrcTexture, uint32_t Width, uint32_t Height);

    FPipelineState TonemappingPipelineState;
    FTexture LDRTexture;
};

#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;

class FMipmapGenerator
{
public:
    FMipmapGenerator(FGraphicsDevice* const Device);

    void GenerateMipmap(FTexture* Texture);

private:
    FGraphicsDevice* Device;
    FPipelineState GenerateMipmapPipelineState;
    FPipelineState GenerateCubemapMipmapPipelineState;
};
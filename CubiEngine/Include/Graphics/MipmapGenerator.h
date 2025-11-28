#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FMipmapGenerator
{
public:
    FMipmapGenerator();

    void GenerateMipmap(FTexture* Texture);

private:
    FPipelineState GenerateMipmapPipelineState;
    FPipelineState GenerateCubemapMipmapPipelineState;
};
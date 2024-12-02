#pragma once

#include "Graphics/PipelineState.h"
struct FCubeMapCreationDesc
{
    std::wstring EquirectangularTexturePath;
    std::wstring Name;
};

class FGraphicsDevice;

class FCubeMap
{
public:
    FCubeMap(FGraphicsDevice* Device, const FCubeMapCreationDesc& Desc);

    FTexture CubeMapTexture;
    FPipelineState ConvertEquirectToCubeMapPipelineState;
};

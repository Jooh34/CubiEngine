#pragma once

#include "Graphics/PipelineState.h"
#include "ShaderInterlop/RenderResources.hlsli"

struct FCubeMapCreationDesc
{
    std::wstring EquirectangularTexturePath;
    std::wstring Name;
};

class FGraphicsDevice;
class FGraphicsContext;

class FCubeMap
{
public:
    FCubeMap(FGraphicsDevice* Device, const FCubeMapCreationDesc& Desc);
    void Render(FGraphicsContext* const GraphicsContext,
        interlop::ScreenSpaceCubeMapRenderResources& RenderResource,
        const FTexture& Target, const FTexture& DepthBuffer);

    FTexture CubeMapTexture;
    FPipelineState ConvertEquirectToCubeMapPipelineState;
    FPipelineState ScreenSpaceCubemapPipelineState;
};

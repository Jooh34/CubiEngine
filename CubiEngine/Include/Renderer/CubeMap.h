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

    void GeneratePrefilteredCubemap(const FCubeMapCreationDesc& Desc, uint32_t MipLevels);
    void GenerateBRDFLut(const FCubeMapCreationDesc& Desc);

    void Render(FGraphicsContext* const GraphicsContext,
        interlop::ScreenSpaceCubeMapRenderResources& RenderResource,
        FTexture& Target, const FTexture& DepthBuffer);

    FTexture CubeMapTexture;
    FTexture PrefilteredCubemapTexture;
    FTexture BRDFLutTexture;

    FPipelineState ConvertEquirectToCubeMapPipelineState;
    FPipelineState ScreenSpaceCubemapPipelineState;
    FPipelineState PrefilterPipelineState;
    FPipelineState BRDFLutPipelineState;

private:
    FGraphicsDevice* Device;
};

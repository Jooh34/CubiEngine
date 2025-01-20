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

    void GeneratePrefilteredCubemap(const FCubeMapCreationDesc& Desc, uint32_t MipCount);
    void GenerateBRDFLut(const FCubeMapCreationDesc& Desc);
    void GenerateIrradianceMap(const FCubeMapCreationDesc& Desc);

    void Render(FGraphicsContext* const GraphicsContext,
        interlop::ScreenSpaceCubeMapRenderResources& RenderResource,
        FTexture& Target, const FTexture& DepthBuffer);

    FTexture CubeMapTexture;
    FTexture PrefilteredCubemapTexture;
    FTexture IrradianceCubemapTexture;
    FTexture BRDFLutTexture;

    FPipelineState ConvertEquirectToCubeMapPipelineState;
    FPipelineState ScreenSpaceCubemapPipelineState;
    FPipelineState PrefilterPipelineState;
    FPipelineState GenerateIrradianceMapPipelineState;
    FPipelineState BRDFLutPipelineState;
    
    uint32_t GetMipCount() const { return MipCount; };

private:
    FGraphicsDevice* Device;

    uint32_t MipCount = 6u;
};

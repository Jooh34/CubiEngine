#pragma once

#include "Graphics/PipelineState.h"
#include "ShaderInterlop/RenderResources.hlsli"

struct FCubeMapCreationDesc
{
    std::wstring EquirectangularTexturePath;
    std::wstring Name;
};

class FGraphicsContext;

class FCubeMap
{
public:
    FCubeMap(const FCubeMapCreationDesc& Desc);

    void GeneratePrefilteredCubemap(const FCubeMapCreationDesc& Desc, uint32_t MipCount);
    void GenerateBRDFLut(const FCubeMapCreationDesc& Desc);
    void GenerateIrradianceMap(const FCubeMapCreationDesc& Desc);

    void Render(FGraphicsContext* const GraphicsContext,
        interlop::ScreenSpaceCubeMapRenderResources& RenderResource,
        FTexture* Target, const FTexture* DepthBuffer);

    std::unique_ptr<FTexture> CubeMapTexture;
    std::unique_ptr<FTexture> PrefilteredCubemapTexture;
    std::unique_ptr<FTexture> IrradianceCubemapTexture;
    std::unique_ptr<FTexture> BRDFLutTexture;

    FPipelineState ConvertEquirectToCubeMapPipelineState;
    FPipelineState ScreenSpaceCubemapPipelineState;
    FPipelineState PrefilterPipelineState;
    FPipelineState GenerateIrradianceMapPipelineState;
    FPipelineState BRDFLutPipelineState;
    
    uint32_t GetMipCount() const { return MipCount; };

private:
    uint32_t MipCount = 6u;
};

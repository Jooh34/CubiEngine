#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FScene;

struct FGBuffer
{
    FTexture Albedo{};
    FTexture NormalEmissive{};
    FTexture AoMetalicRoughness{};
};

class FDeferredGPass
{
public:
    FDeferredGPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);
    void Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        const FTexture& DepthBuffer, uint32_t Width, uint32_t Height);

    FGBuffer GBuffer;

private:
    FPipelineState PipelineState;
};
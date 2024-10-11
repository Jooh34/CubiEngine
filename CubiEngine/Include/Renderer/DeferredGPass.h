#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;

struct FGBuffer
{
    FTexture Albedo{};
    FTexture NormalEmissive{};
    FTexture AoMetalicRoughness{};
};

class DeferredGPass
{
public:
    DeferredGPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);

    FGBuffer GBuffer;

private:
    FPipelineState PipelineState;
};
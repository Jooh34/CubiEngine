#pragma once

#include "ShaderInterlop/ConstantBuffers.hlsli"

struct FPBRMaterial
{
    FTexture AlbedoTexture{};
    FSampler AlbedoSampler{};

    FTexture NormalTexture{};
    FSampler NormalSampler{};

    FTexture MetalRoughnessTexture{};
    FSampler MetalRoughnessSampler{};

    FTexture AOTexture{};
    FSampler AOSampler{};

    FTexture EmissiveTexture{};
    FSampler EmissiveSampler{};

    FBuffer MaterialBuffer{};
    interlop::MaterialBuffer MaterialBufferData;

    uint32_t MaterialIndex{};

    EAlphaMode AlphaMode;
    double AlphaCutoff;
};
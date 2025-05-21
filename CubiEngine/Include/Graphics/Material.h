#pragma once

#include "ShaderInterlop/ConstantBuffers.hlsli"

class FPBRMaterial
{
public:
    std::string Name{};

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

    FTexture ORMTexture{}; // Bistro
    FSampler ORMSampler{}; // Bistro

    FBuffer MaterialBuffer{};
    interlop::MaterialBuffer MaterialBufferData;

    uint32_t MaterialIndex{};

    EAlphaMode AlphaMode;
    double AlphaCutoff;
};
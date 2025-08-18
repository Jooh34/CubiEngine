#pragma once

#include "ShaderInterlop/ConstantBuffers.hlsli"

class FPBRMaterial
{
public:
    std::string Name{};

    uint32_t GetAlbedoSrv() const { return AlbedoTexture ? AlbedoTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> AlbedoTexture;
    FSampler AlbedoSampler{};

	uint32_t GetNormalSrv() const { return NormalTexture ? NormalTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> NormalTexture;
    FSampler NormalSampler{};

	uint32_t GetMetalRoughnessSrv() const { return MetalRoughnessTexture ? MetalRoughnessTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> MetalRoughnessTexture;
    FSampler MetalRoughnessSampler{};

	uint32_t GetAOTextureSrv() const { return AOTexture ? AOTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> AOTexture;
    FSampler AOSampler{};

	uint32_t GetEmissiveSrv() const { return EmissiveTexture ? EmissiveTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> EmissiveTexture;
    FSampler EmissiveSampler{};

	uint32_t GetORMTextureSrv() const { return ORMTexture ? ORMTexture->SrvIndex : INVALID_INDEX_U32; };
    std::unique_ptr<FTexture> ORMTexture;
    FSampler ORMSampler{};

    FBuffer MaterialBuffer{};
    interlop::MaterialBuffer MaterialBufferData;

    uint32_t MaterialIndex{};

    EAlphaMode AlphaMode;
    double AlphaCutoff;
};
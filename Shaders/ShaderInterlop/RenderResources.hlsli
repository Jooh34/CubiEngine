// clang-format off
#pragma once

#ifdef __cplusplus
    #define uint uint32_t
    #define float2 XMFLOAT2
#endif

namespace interlop
{
    struct TriangleRenderResources
    {
        uint sceneBufferIndex;
        uint positionBufferIndex;
        uint textureCoordBufferIndex;
        uint textureIndex;
    };

    struct UnlitPassRenderResources
    {
        uint positionBufferIndex;
        uint textureCoordBufferIndex;

        uint transformBufferIndex;

        uint sceneBufferIndex;

        uint albedoTextureIndex;
        uint albedoTextureSamplerIndex;

        uint materialBufferIndex;
    };

    struct DeferredGPassRenderResources
    {
        uint positionBufferIndex;
        uint textureCoordBufferIndex;
        uint normalBufferIndex;

        uint transformBufferIndex;

        uint sceneBufferIndex;

        uint albedoTextureIndex;
        uint albedoTextureSamplerIndex;

        uint metalRoughnessTextureIndex;
        uint metalRoughnessTextureSamplerIndex;

        uint normalTextureIndex;
        uint normalTextureSamplerIndex;

        uint aoTextureIndex;
        uint aoTextureSamplerIndex;

        uint emissiveTextureIndex;
        uint emissiveTextureSamplerIndex;

        uint materialBufferIndex;
    };

    struct CopyRenderResources
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
    };

    struct PBRRenderResources
    {
        uint GBufferAIndex;
        uint GBufferBIndex;
        uint GBufferCIndex;
        
        uint outputTextureIndex;
    };
}

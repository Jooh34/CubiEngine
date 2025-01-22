// clang-format off
#pragma once

#ifdef __cplusplus
    #define uint uint32_t
    #define float2 XMFLOAT2
    #define float4x4 XMMATRIX
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

    struct TonemappingRenderResources
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        uint width;
        uint height;

        uint bToneMapping;
        uint bGammaCorrection;
    };

    struct PBRRenderResources
    {
        float4x4 lightViewProjectionMatrix;

        uint GBufferAIndex;
        uint GBufferBIndex;
        uint GBufferCIndex;
        uint depthTextureIndex;

        uint prefilteredEnvmapIndex;
        uint cubemapTextureIndex;
        uint envBRDFTextureIndex;
        uint envIrradianceTextureIndex;
        uint envMipCount;

        uint shadowDepthTextureIndex;

        uint outputTextureIndex;

        uint width;
        uint height;
        uint sceneBufferIndex;
        uint lightBufferIndex;

        uint bUseEnvmap;
        uint bUseEnergyCompensation;
        uint WhiteFurnaceMethod;
        uint sampleBias;
    };

    struct ConvertEquirectToCubeMapRenderResource
    {
        uint textureIndex;
        uint outputCubeMapTextureIndex;
    };

    struct DebugVisualizeCubeMapRenderResources
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
    };

    struct DebugVisualizeDepthRenderResources
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
    };

    struct GenerateMipmapResource
    {
        uint srcMipSrvIndex;
        uint srcMipLevel;
        float2 dstTexelSize;

        uint dstMipIndex;
        uint isSRGB;
    };

    struct ScreenSpaceCubeMapRenderResources
    {
        uint sceneBufferIndex;
        uint cubenmapTextureIndex;
    };

    struct GeneratePrefilteredCubemapResource
    {
        uint srcMipSrvIndex;
        uint dstMipUavIndex;
        uint mipLevel;
        uint totalMipLevel;
        
        float2 texelSize;
    };

    struct GenerateBRDFLutRenderResource
    {
        uint textureUavIndex;
    };

    struct ShadowDepthPassRenderResource
    {
        float4x4 lightViewProjectionMatrix;

        uint positionBufferIndex;
        uint transformBufferIndex;
    };
}

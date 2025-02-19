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

        uint debugBufferIndex;
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

    struct DeferredGPassCubeRenderResources
    {
        float4x4 modelMatrix;
        float4x4 inverseModelMatrix;
        uint debugBufferIndex;
        uint sceneBufferIndex;
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
        uint numCascadeShadowMap;

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
        uint shadowBufferIndex;

        uint bUseEnvmap;
        uint bUseEnergyCompensation;
        uint WhiteFurnaceMethod;
        uint bCSMDebug;
        uint sampleBias;
    };

    struct ConvertEquirectToCubeMapRenderResource
    {
        uint textureIndex;
        uint outputCubeMapTextureIndex;
    };

    struct DebugVisualizeRenderResources
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

    struct TemporalAAResolveRenderResource
    {
        uint sceneTextureIndex;
        uint historyTextureIndex;
        uint velocityTextureIndex;
        uint dstTextureIndex;
        uint width;
        uint height;
        uint historyFrameCount;
    };

    struct TemporalAAUpdateHistoryRenderResource
    {
        uint resolveTextureIndex;
        uint historyTextureIndex;
        uint width;
        uint height;
    };

    struct GenerateStochasticNormalRenderResource
    {
        uint srcTextureIndex;
        uint gbufferCTextureIndex;
        uint dstTextureIndex;
        uint width;
        uint height;
        
        uint frameCount;
        uint stochasticNormalSamplingMethod;
    };

    struct RaycastDiffuseRenderResource
    {
        uint sceneColorTextureIndex;
        uint depthTextureIndex;
        uint stochasticNormalTextureIndex;
        uint dstTextureIndex;

        uint sceneBufferIndex;
        uint width;
        uint height;
        float rayLength;

        uint numSteps;
        float ssgiIntensity;
    };

    struct CompositionSSGIRenderResource
    {
        uint resolveTextureIndex;
        uint gbufferAIndex;
        uint dstTextureIndex;
        uint width;
        uint height;
    };

    struct DownSampleResource
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        float2 dstTexelSize;
    };

    struct UpSampleResource
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        float2 dstTexelSize;
    };

    struct GaussianBlurRenderResource
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        float2 dstTexelSize;

        uint bHorizontal;
    };
}

// clang-format off
#pragma once

#ifdef __cplusplus
    #define uint uint32_t
    #define float2 XMFLOAT2
    #define float3 XMFLOAT3
    #define float4 XMFLOAT4
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
        uint tangentBufferIndex;
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
        float2 padding;

        float4 lightColor;
        float4 intensityDistance;
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
        uint bloomTextureIndex;
        uint width;
        uint height;

        uint toneMappingMethod;
        uint bGammaCorrection;
        uint averageLuminanceBufferIndex;
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
        uint vsmMomentTextureIndex;
        uint ssaoTextureIndex;

        uint outputTextureIndex;

        float2 dstTexelSize;
        uint sceneBufferIndex;
        uint lightBufferIndex;
        uint shadowBufferIndex;
        uint debugBufferIndex;

        uint bUseEnvmap;
        uint bUseEnergyCompensation;
        uint WhiteFurnaceMethod;
        uint bCSMDebug;
        uint diffuseMethod;
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
        uint srcWidth;
        uint srcHeight;

        float visDebugMin;
        float visDebugMax;
    };

    struct DebugVisualizeDepthRenderResources
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        float visDebugMin;
        float visDebugMax;
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
        float2 dstTexelSize;
        uint historyFrameCount;
    };

    struct TemporalAAUpdateHistoryRenderResource
    {
        uint resolveTextureIndex;
        uint historyTextureIndex;
        float2 dstTexelSize;
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
        uint GBufferBTextureIndex;
        uint dstTextureIndex;

        uint sceneBufferIndex;
        uint width;
        uint height;
        float rayLength;

        uint numSteps;
        float ssgiIntensity;
        float compareToleranceScale;
        uint frameCount;

        uint stochasticNormalSamplingMethod;
        uint numSamples;
    };

    struct CompositionSSGIRenderResource
    {
        uint resolveTextureIndex;
        uint gbufferAIndex;
        uint dstTextureIndex;
        uint width;
        uint height;
    };

    struct SSGIUpdateHistoryRenderResource
    {
        uint resolveTextureIndex;
        uint historyTextureIndex;
        float2 dstTexelSize;
    };

    
    struct SSGIResolveRenderResource
    {
        uint denoisedTextureIndex;
        uint historyTextureIndex;
        uint velocityTextureIndex;
        uint prevDepthTextureIndex;

        uint depthTextureIndex;
        uint dstTextureIndex;
        uint numFramesAccumulatedTextureIndex;
        uint sceneBufferIndex;
        
        float2 dstTexelSize;
        uint maxHistoryFrame;
    };

    struct DownSampleRenderResource
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

    static const uint MAX_GAUSSIAN_KERNEL_SIZE_DIV_4 = 8u;
    struct GaussianBlurWRenderResource
    {
        uint srcTextureIndex;
        uint dstTextureIndex;
        float2 dstTexelSize;
    
        uint additiveTextureIndex;
        uint bHorizontal;
        uint kernelSize;
        float padding;

        float4 weights[MAX_GAUSSIAN_KERNEL_SIZE_DIV_4];
    };

    struct GenerateHistogramRenderResource
    {
        uint sceneTextureIndex;
        uint histogramBufferIndex;
        float minLogLuminance;
        float inverseLogLuminanceRange;
    };

    struct CalculateAverageLuminanceRenderResource
    {
        uint histogramBufferIndex;
        uint averageLuminanceBufferIndex;
        float minLogLuminance;
        float logLumRange;
        float timeCoeff;
        uint numPixels;
    };

    struct SSAORenderResource
    {
        uint GBufferBIndex;
        uint depthTextureIndex;
        uint dstTextureIndex;
        uint SSAOKernelBufferIndex;

        uint sceneBufferIndex;
        uint frameCount;
        uint kernelSize;
        float kernelRadius;

        float depthBias;
        uint bUseRangeCheck;
    };

    struct VSMMomentPassRenderResource
    {
        uint srcTextureIndex;
        uint momentTextureIndex;
        float2 dstTexelSize;
    };

    // RayTracing
    struct RTSceneDebugRenderResource
    {
        float4x4 invViewProjectionMatrix;
        uint dstTextureIndex;
        uint geometryInfoBufferIdx;
        uint materialBufferIdx;
        uint vtxBufferIdx;
        uint idxBufferIdx;

        float3 padding;
    };
}

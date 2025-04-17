// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::TonemappingRenderResources> renderResources : register(b0);

float4 Reinhard(float4 x) {
    return x / (1.0 + x);
}

float4 ReinhardModifed(float4 x) {
    const float L_white = 4.0;
    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

float4 Tonemap_ACES(float4 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

#define TONEMAPPING_OFF 0
#define TONEMAPPING_REINHARD 1
#define TONEMAPPING_REINHARD_MODIFIED 2
#define TONEMAPPING_ACES 3

[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;
    
    float4 color = srcTexture.Sample(pointClampSampler, uv);
    
    if (renderResources.bloomTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> bloomTexture = ResourceDescriptorHeap[renderResources.bloomTextureIndex];
        color.xyz = color.xyz + bloomTexture.Sample(pointClampSampler, uv).xyz;
    }

    if (renderResources.averageLuminanceBufferIndex != INVALID_INDEX)
    {
        StructuredBuffer<float4> averageLuminanceBuffer = ResourceDescriptorHeap[renderResources.averageLuminanceBufferIndex];
        float averageLuminance = averageLuminanceBuffer[0].x;
        color = color / (9.6 * averageLuminance);
    }

    if (renderResources.toneMappingMethod == TONEMAPPING_REINHARD)
    {
        color = Reinhard(color);
    }
    else if (renderResources.toneMappingMethod == TONEMAPPING_REINHARD_MODIFIED)
    {
        color = ReinhardModifed(color);
    }
    else if (renderResources.toneMappingMethod == TONEMAPPING_ACES)
    {
        color = Tonemap_ACES(color);
    }
    else
    {
        color = clamp(color, 0, 1);
    }

    if (renderResources.bGammaCorrection)
    {
        float gamma = rcp(2.2f);
        color = pow(color, gamma);
    }

    dstTexture[dispatchThreadID.xy] = color;
}
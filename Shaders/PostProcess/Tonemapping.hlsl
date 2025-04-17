// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::TonemappingRenderResources> renderResources : register(b0);

float Reinhard(float x) {
    return x / (1.0 + x);
}

float Reinhard2(float x) {
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


[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;
    
    float4 color = srcTexture.Sample(pointClampSampler, uv);

    if (renderResources.toneMappingMethod != 0)
    {
        color = Tonemap_ACES(color);
    }

    if (renderResources.bGammaCorrection)
    {
        float gamma = rcp(2.2f);
        color = pow(color, gamma);
    }

    dstTexture[dispatchThreadID.xy] = color;
}
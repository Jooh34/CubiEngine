// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::GenerateMipmapResource> renderResources : register(b0);

float3 convertToLinear(float3 x)
{
    return all(x < 0.04045f) ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

float3 convertToSRGB(float3 x)
{
    return all(x < 0.0031308) ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}

[numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float4> srcMipTexture = ResourceDescriptorHeap[renderResources.srcMipSrvIndex];
    RWTexture2D<float4> dstMipTexture = ResourceDescriptorHeap[renderResources.dstMipIndex];
    uint srcMipLevel = renderResources.srcMipLevel;

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);
    
    // Map destination pixel to 4 source texels
    float2 srcCoord00 = uvCoords + texelSize * float2(-0.5f, -0.5f);
    float2 srcCoord10 = uvCoords + texelSize * float2(0.5f, -0.5f);
    float2 srcCoord01 = uvCoords + texelSize * float2(-0.5f, 0.5f);
    float2 srcCoord11 = uvCoords + texelSize * float2(0.5f, 0.5f);

    // Fetch and average
    float4 c00 = srcMipTexture.SampleLevel(linearClampSampler, srcCoord00, srcMipLevel);
    float4 c10 = srcMipTexture.SampleLevel(linearClampSampler, srcCoord10, srcMipLevel);
    float4 c01 = srcMipTexture.SampleLevel(linearClampSampler, srcCoord01, srcMipLevel);
    float4 c11 = srcMipTexture.SampleLevel(linearClampSampler, srcCoord11, srcMipLevel);
    float4 averagedColor = (c00 + c10 + c01 + c11) * 0.25f;

    if (renderResources.isSRGB)
    {
        averagedColor = float4(convertToSRGB(averagedColor.rgb), averagedColor.a);
    }
    dstMipTexture[coord] = averagedColor;
}
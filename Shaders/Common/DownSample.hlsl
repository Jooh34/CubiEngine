// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::DownSampleRenderResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);
    
    // Map destination pixel to 4 source texels
    // float2 srcCoord00 = uvCoords + texelSize * float2(-0.5f, -0.5f);
    // float2 srcCoord10 = uvCoords + texelSize * float2(0.5f, -0.5f);
    // float2 srcCoord01 = uvCoords + texelSize * float2(-0.5f, 0.5f);
    // float2 srcCoord11 = uvCoords + texelSize * float2(0.5f, 0.5f);

    // // Fetch and average
    // float4 c00 = srcTexture.Sample(linearClampSampler, srcCoord00);
    // float4 c10 = srcTexture.Sample(linearClampSampler, srcCoord10);
    // float4 c01 = srcTexture.Sample(linearClampSampler, srcCoord01);
    // float4 c11 = srcTexture.Sample(linearClampSampler, srcCoord11);
    // float4 averagedColor = (c00 + c10 + c01 + c11) * 0.25f;

    float4 averagedColor = srcTexture.Sample(linearClampSampler, uvCoords);
    dstTexture[coord] = averagedColor;
}
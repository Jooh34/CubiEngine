// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::GenerateMipmapResource> renderResources : register(b0);

[numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;
    uint faceIndex = dispatchThreadID.z;

    Texture2DArray<float4> srcMipTexture = ResourceDescriptorHeap[renderResources.srcMipSrvIndex];
    RWTexture2DArray<float4> dstMipTexture = ResourceDescriptorHeap[renderResources.dstMipIndex];
    uint srcMipLevel = renderResources.srcMipLevel;

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);
    
    // Map destination pixel to 4 source texels
    float2 srcCoord00 = uvCoords + texelSize * float2(-0.5f, -0.5f);
    float2 srcCoord10 = uvCoords + texelSize * float2(0.5f, -0.5f);
    float2 srcCoord01 = uvCoords + texelSize * float2(-0.5f, 0.5f);
    float2 srcCoord11 = uvCoords + texelSize * float2(0.5f, 0.5f);

    // Fetch and average
    float4 c00 = srcMipTexture.SampleLevel(linearClampSampler, float3(srcCoord00, faceIndex), srcMipLevel);
    float4 c10 = srcMipTexture.SampleLevel(linearClampSampler, float3(srcCoord10, faceIndex), srcMipLevel);
    float4 c01 = srcMipTexture.SampleLevel(linearClampSampler, float3(srcCoord01, faceIndex), srcMipLevel);
    float4 c11 = srcMipTexture.SampleLevel(linearClampSampler, float3(srcCoord11, faceIndex), srcMipLevel);

    float4 averagedColor = (c00 + c10 + c01 + c11) * 0.25f;

    dstMipTexture[dispatchThreadID] = averagedColor;
}
// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::UpSampleResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);

    float4 color = srcTexture.Sample(linearClampSampler, uvCoords);
    dstTexture[coord] = color;
}
// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::DebugVisualizeRenderResources> renderResources : register(b0);


[numthreads(8, 8, 1)]

void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    
    float dstWidth, dstHeight;
    dstTexture.GetDimensions(dstWidth, dstHeight);
    
    uint2 coord = dispatchThreadID.xy;
    float2 texelSize = float2(1.f/dstWidth, 1.f/dstHeight);
    float2 uvCoords = texelSize * (coord + 0.5);

    float4 color = srcTexture.Sample(linearClampSampler, uvCoords);

    color = (color - renderResources.visDebugMin) / (renderResources.visDebugMax - renderResources.visDebugMin);

    dstTexture[dispatchThreadID.xy] = color;
}
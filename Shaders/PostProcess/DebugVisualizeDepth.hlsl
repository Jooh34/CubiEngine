// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::DebugVisualizeDepthRenderResources> renderResources : register(b0);


[numthreads(8, 8, 1)]

void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 dstDimension = float2(0.0f, 0.0f);
    dstTexture.GetDimensions(dstDimension.x, dstDimension.y);
    float2 invViewport = float2(1.f/(float)dstDimension.x, 1.f/(float)dstDimension.y);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;
    
    float depth = srcTexture.Sample(linearClampSampler, uv);
    depth = (depth - renderResources.visDebugMin) / (renderResources.visDebugMax - renderResources.visDebugMin);

    dstTexture[dispatchThreadID.xy] = depth;
}
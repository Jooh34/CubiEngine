// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::SSGIUpdateHistoryRenderResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> resolveTexture = ResourceDescriptorHeap[renderResources.resolveTextureIndex];
    RWTexture2D<float4> historyTexture = ResourceDescriptorHeap[renderResources.historyTextureIndex];
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    float3 color = resolveTexture.Sample(pointClampSampler, uv).xyz;

    historyTexture[dispatchThreadID.xy] = float4(color, 1.f);
}
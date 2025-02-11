// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::TemporalAAResolveRenderResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> sceneTexture = ResourceDescriptorHeap[renderResources.sceneTextureIndex];
    Texture2D<float4> historyTexture = ResourceDescriptorHeap[renderResources.historyTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    float3 sceneColor = sceneTexture.Sample(pointClampSampler, uv).xyz;
    float3 historyColor = historyTexture.Sample(linearWrapSampler, uv).xyz;
    
    float modulationFactor = 0.9;
    float3 color = lerp(sceneColor, historyColor, modulationFactor);
    if (renderResources.historyFrameCount < 1)
    {
        color = sceneColor;
    }
    
    dstTexture[dispatchThreadID.xy] = float4(color, 1.f);
}
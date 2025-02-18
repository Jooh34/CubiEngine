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
    Texture2D<float2> velocityTexture = ResourceDescriptorHeap[renderResources.velocityTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;
    float2 velocity = velocityTexture.Sample(linearWrapSampler, uv);
    const float2 prevUV = uv - velocity;

    float3 sceneColor = sceneTexture.Sample(pointClampSampler, uv).xyz;
    float3 historyColor = historyTexture.Sample(pointClampSampler, prevUV).xyz;
    
    float3 nearColor0 = sceneTexture.Sample(pointClampSampler, uv, int2(1, 0)).xyz;
    float3 nearColor1 = sceneTexture.Sample(pointClampSampler, uv, int2(0, 1)).xyz;
    float3 nearColor2 = sceneTexture.Sample(pointClampSampler, uv, int2(-1, 0)).xyz;
    float3 nearColor3 = sceneTexture.Sample(pointClampSampler, uv, int2(0, -1)).xyz;
    
    float3 boxMin = min(sceneColor, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
    float3 boxMax = max(sceneColor, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));;

    historyColor = clamp(historyColor, boxMin, boxMax);

    float modulationFactor = 0.9;
    float3 color = lerp(sceneColor, historyColor, modulationFactor);
    if (renderResources.historyFrameCount < 1)
    {
        color = sceneColor;
    }
    
    dstTexture[dispatchThreadID.xy] = float4(color, 1.f);
}
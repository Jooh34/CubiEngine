// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
ConstantBuffer<interlop::CompositionSSGIRenderResource> renderResources : register(b0);

[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> resolveTexture = ResourceDescriptorHeap[renderResources.resolveTextureIndex];
    Texture2D<float4> GBufferATexture = ResourceDescriptorHeap[renderResources.gbufferAIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport; 
    const float2 pixel = (dispatchThreadID.xy + 0.5f); 

	float4 ssgi = resolveTexture.Sample(linearWrapSampler, uv);
    float4 albedo = GBufferATexture.Sample(linearWrapSampler, uv);
    dstTexture[dispatchThreadID.xy] = float4(dstTexture[dispatchThreadID.xy].xyz + ssgi.xyz * albedo.xyz, 1.f);
}
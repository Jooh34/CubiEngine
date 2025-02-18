// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
ConstantBuffer<interlop::GenerateStochasticNormalRenderResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    Texture2D<float4> GBufferC = ResourceDescriptorHeap[renderResources.gbufferCTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport; 
    const float2 pixel = (dispatchThreadID.xy + 0.5f);

    const float4 normal = srcTexture.Sample(pointClampSampler, uv);
    const float4 aoMetalRoughness = GBufferC.Sample(pointClampSampler, uv);
    float roughness = aoMetalRoughness.z;

    float noise = InterleavedGradientNoise(pixel, renderResources.frameCount);
    float2 u = float2(noise, noise);
    // float2 u = Hammersley( dispatchThreadID.x * renderResources.height + dispatchThreadID.y, invViewport.x * invViewport.y );
    
    float3 CosDirNormal;
    float NdotL;
    float pdf;
    ImportanceSampleCosDir(u, normal.xyz, CosDirNormal, NdotL, pdf);
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);
    float3 UniformNormal = tangentToWorldCoords(UniformSampleHemisphere(u), normal.xyz, s, t);

    float3 stochasticNormal = roughness * UniformNormal + (1-roughness) * CosDirNormal;

    dstTexture[dispatchThreadID.xy] = float4(stochasticNormal, 1);
}
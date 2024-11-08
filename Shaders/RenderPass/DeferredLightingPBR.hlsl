#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numThreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> GBufferA = ResourceDescriptorHeap[renderResources.GBufferAIndex];
    Texture2D<float4> GBufferB = ResourceDescriptorHeap[renderResources.GBufferBIndex];
    Texture2D<float4> GBufferC = ResourceDescriptorHeap[renderResources.GBufferCIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];
    
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    const float depth = depthTexture.Sample(pointClampSampler, uv);
    const float4 albedo = GBufferA.Sample(pointClampSampler, uv);
    const float4 normal = GBufferB.Sample(pointClampSampler, uv);
    const float4 metalRoughness = GBufferC.Sample(pointClampSampler, uv);
    // const float4 ao = ResourceDescriptorHeap[renderResources.aoTextureIndex].Sample(pointClampSampler, uv);
    // const float4 emissive = ResourceDescriptorHeap[renderResources.emissiveTextureIndex].Sample(pointClampSampler, uv);

    if (depth > 0.9999f) return;

    // temporal constant value for directional light
    const float3 L = float3(1.f,1.f,1.f);
    const float3 V = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);
    const float3 H = normalize(V+L);
    const float3 N = normal.xyz;

    BxDFContext context = (BxDFContext)0;
    context.VoH = max(dot(V,H), 0.f);
    context.NoV = max(dot(N,V), 0.f); 
    context.NoL = max(dot(N,L), 0.f);
    context.NoH = max(dot(N,H), 0.f);

    float3 color = float3(0,0,0);
    //color += diffuseLambert(albedo.xyz);
    color += specularGGX(albedo.xyz, metalRoughness.y, metalRoughness.x, context);

    outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
}
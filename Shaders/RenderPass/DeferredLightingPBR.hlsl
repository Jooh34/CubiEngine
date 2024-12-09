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

    TextureCube<float4> PrefilterEnvmap = ResourceDescriptorHeap[renderResources.prefilteredEnvmapIndex];
    Texture2D<float2> EnvBRDFTexture = ResourceDescriptorHeap[renderResources.envBRDFTextureIndex];
    
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    const float depth = depthTexture.Sample(pointClampSampler, uv);
    const float4 albedo = GBufferA.Sample(pointClampSampler, uv);
    const float4 normal = GBufferB.Sample(pointClampSampler, uv);
    const float4 aoMetalRoughness = GBufferC.Sample(pointClampSampler, uv);
    float roughness = aoMetalRoughness.z;
    float metalic = aoMetalRoughness.y;

    if (depth > 0.9999f) return;

    // temporal constant value for directional light
    const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[0].xyz);
    const float3 V = normalize(-viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix));
    const float3 H = normalize(V+L);
    const float3 N = normal.xyz;

    BxDFContext context = (BxDFContext)0;
    context.VoH = max(dot(V,H), 0.f);
    context.NoV = max(dot(N,V), 0.f); 
    context.NoL = max(dot(N,L), 0.f);
    context.NoH = max(dot(N,H), 0.f);

    float3 color = float3(0,0,0);
    color += cookTorrence(albedo.xyz, roughness, metalic, context) * 10.f; // TODO : light color, attenuation

    // IBL : PrefilteredEnvMap from UE4
    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metalic);
    
    const float3 R = normalize(mul(reflect(-V, N), (float3x3)sceneBuffer.inverseViewMatrix));
    const float3 PrefilteredColor = PrefilterEnvmap.SampleLevel(minMapLinearMipPointClampSampler, R, roughness * 6.0f).xyz;
    const float2 EnvBRDF = EnvBRDFTexture.Sample(pointWrapSampler, float2(roughness, context.NoV));
    const float3 specularIBL = PrefilteredColor * (f0 * EnvBRDF.x + EnvBRDF.y);
    color += specularIBL;
    
    outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
}
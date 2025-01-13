#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"
#include "Shadow/Shadow.hlsl"

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

    TextureCube<float4> CubeMapTexture = ResourceDescriptorHeap[renderResources.cubemapTextureIndex];
    TextureCube<float4> PrefilterEnvmap = ResourceDescriptorHeap[renderResources.prefilteredEnvmapIndex];
    Texture2D<float4> EnvBRDFTexture = ResourceDescriptorHeap[renderResources.envBRDFTextureIndex];
    
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

    const float3 worldSpaceNormal = normalize(mul(normal.xyz, (float3x3)sceneBuffer.inverseViewMatrix));

    if (depth > 0.9999f) return;

    const float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);
    const float3 V = normalize(-viewSpacePosition);
    const float3 N = normal.xyz;
    
    BxDFContext context = (BxDFContext)0;
    context.NoV = max(dot(N,V), 0.f); 

    float3 color = float3(0,0,0);
    {
        // Directional Light
        uint DLIndex = 0;
        float4 lightColor = lightBuffer.lightColor[DLIndex];
        float lightIntensity = lightBuffer.intensity[DLIndex];

        const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[DLIndex].xyz);
        const float3 H = normalize(V+L);

        context.VoH = max(dot(V,H), 0.f);
        context.NoH = max(dot(N,H), 0.f);
        context.NoL = max(dot(N,L), 0.f);

        // shadow
        const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), sceneBuffer.inverseViewMatrix);
        const float4 lightSpacePosition = mul(worldSpacePosition, renderResources.lightViewProjectionMatrix);

        const float shadow = calculateShadow(lightSpacePosition, context.NoL, renderResources.shadowDepthTextureIndex);
        const float attenuation = (1.f - shadow);
        color += attenuation * lightColor.xyz * lightIntensity * cookTorrence(albedo.xyz, roughness, metalic, context); // TODO : light color, attenuation
    }

    // IBL : PrefilteredEnvMap from UE4
    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metalic);
    
    float MAX_MIPLEVEL = 12.f;
    int EnvResolution = 1024;
    uint numSample = 128u;
    float InvNumSamples = 1.f / numSample;

    float3 accBrdf = float3(0,0,0);
    float accBrdfWeight = 0.f;
    float3 accDiffuseBrdf = float3(0,0,0);
    for (uint i = 0u; i < numSample; ++i)
    {
        float2 Xi = Hammersley( i, InvNumSamples );
        float3 H = ImportanceSampleGGX( Xi, roughness, N);
        float3 L = reflect(-V, H);
        float sampledNoL = dot(N, L);
        if (sampledNoL > 0)
        {
            float sampledNoH = saturate(dot(N, H));
            float sampledLoH = saturate(dot(L, H));
            float D = D_GGX(roughness*roughness, sampledNoH);
            float pdf = D * sampledNoH /(4* sampledLoH * PI + EPS);
            float omegaS = 1.0 / ( numSample * pdf + EPS);
            float omegaP = 4.0 * PI / (6.0 * EnvResolution * EnvResolution ) ;
            float mipLevel = roughness == 0.0 ? 0.0 : clamp(0.5 * log2 ( omegaS / omegaP) , 0, MAX_MIPLEVEL);
            float3 Li = CubeMapTexture.SampleLevel(pointClampSampler, L, mipLevel).rgb;
            accBrdf += Li * sampledNoL;
            accBrdfWeight += sampledNoL;
        }

        // diffuse term
        Xi = frac(Xi + 0.5);
        float pdf;
        float nDotL;
        ImportanceSampleCosDir(Xi, N, L, nDotL, pdf);
        if (nDotL > 0.0)
        {
            accDiffuseBrdf += CubeMapTexture.SampleLevel(pointClampSampler, L, 7).rgb;
        }
    }

    float3 SpecularLD = accBrdf * (1.f / accBrdfWeight);
    float3 DiffuseLD = accDiffuseBrdf * (1.f / numSample);

    const float4 EnvBRDF = EnvBRDFTexture.Sample(pointWrapSampler, float2(roughness, context.NoV));
    const float3 specularDFG = (f0 * EnvBRDF.x + EnvBRDF.y);
    const float diffuseDFG = EnvBRDF.z;

    color += specularDFG * SpecularLD;
    //color += diffuseDFG * DiffuseLD;
    
    outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
}
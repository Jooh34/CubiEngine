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
    TextureCube<float4> IrradianceTexture = ResourceDescriptorHeap[renderResources.envIrradianceTextureIndex];
    uint EnvMipCount =  renderResources.envMipCount;
    
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];

    uint sampleBias = renderResources.sampleBias;

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    const float depth = depthTexture.Sample(pointClampSampler, uv);
    float4 albedo = GBufferA.Sample(pointClampSampler, uv);
    if (renderResources.bWhiteFurnace)
    {
        albedo = float4(1,1,1,1);
    }
    
    const float4 normal = GBufferB.Sample(pointClampSampler, uv);
    const float4 aoMetalRoughness = GBufferC.Sample(pointClampSampler, uv);
    float roughness = aoMetalRoughness.z;
    float metalic = aoMetalRoughness.y;
    
    // https://google.github.io/filament/Filament.md.html#figure_roughness_remap
    roughness = max(roughness, 0.089f);

    const float3 worldSpaceNormal = normalize(mul(normal.xyz, (float3x3)sceneBuffer.inverseViewMatrix));

    if (depth > 0.9999f) return;

    const float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);
    const float3 V = normalize(-viewSpacePosition);
    const float3 N = normal.xyz;
    
    BxDFContext context = (BxDFContext)0;
    context.NoV = max(dot(N,V), 0.f); 

    float3 color = float3(0,0,0);

    // Directional Light
    uint DLIndex = 0;
    float4 lightColor = lightBuffer.lightColor[DLIndex];
    float lightIntensity = lightBuffer.intensity[DLIndex];
    if (lightIntensity > 0.f)
    {
        const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[DLIndex].xyz);
        const float3 H = normalize(V+L);

        context.VoH = saturate(dot(V,H));
        context.NoH = saturate(dot(N,H));
        context.NoL = saturate(dot(N,L));

        // shadow
        const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), sceneBuffer.inverseViewMatrix);
        const float4 lightSpacePosition = mul(worldSpacePosition, renderResources.lightViewProjectionMatrix);

        //onst float shadow = calculateShadow(lightSpacePosition, context.NoL, renderResources.shadowDepthTextureIndex);
        const float attenuation = 1.f;
        color += attenuation * lightColor.xyz * lightIntensity * cookTorrence(albedo.xyz, roughness, metalic, context); // TODO : light color, attenuation
    }
    
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metalic);
    
    float3 L = reflect(-V, N);
    float3 WorldSpaceL = normalize(mul(L, (float3x3)sceneBuffer.inverseViewMatrix));
    if (renderResources.bUseEnvmapSpecular)
    {
        // Envmap Specular
        uint numSample = 1024;
        int EnvResolution = 1024;
        uint PrefilterMipLevel = min(roughness * EnvMipCount, EnvMipCount-1);
        float3 SpecularLD = PrefilterEnvmap.SampleLevel(pointClampSampler, WorldSpaceL, PrefilterMipLevel).rgb;
        
        float NoV = saturate(dot(N, V));
        const float4 EnvBRDF = EnvBRDFTexture.Sample(linearClampSampler, float2(NoV, roughness));
        const float3 specularDFG = (F0 * EnvBRDF.x + EnvBRDF.yyy);
        
        color += specularDFG * SpecularLD;
    }

    if (renderResources.bUseEnvmapDiffuse)
    {
        // Envmap Diffuse
        const float3 irradiance = IrradianceTexture.Sample(pointWrapSampler, N).rgb;
        float3 diffuseColor = (1-metalic) * albedo.xyz;
        color += (diffuseColor * irradiance);
        // TODO
    }
    
    //Multi-scattering
    
    // if (renderResources.bUseEnvmapSpecular)
    // {
    //     float NoV = saturate(dot(N, V));
    //     const float4 EnvBRDF = EnvBRDFTexture.Sample(linearClampSampler, float2(NoV, roughness));
    //     const float3 irradiance = IrradianceTexture.Sample(pointWrapSampler, N).rgb;
    //     uint PrefilterMipLevel = min(roughness * EnvMipCount, EnvMipCount-1);
    //     float3 radiance = PrefilterEnvmap.SampleLevel(pointClampSampler, WorldSpaceL, PrefilterMipLevel).rgb;

    //     float3 splat = float3(1-roughness, 1-roughness, 1-roughness);
    //     float3 Fr = max(splat, F0) - F0;
    //     float3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);

    //     float3 FssEss = k_S * EnvBRDF.x + EnvBRDF.y;

    //     // Multiple scattering, from Fdez-Aguera
    //     float Ess = EnvBRDF.x + EnvBRDF.y;
    //     float Ems = (1.0 - Ess);
    //     float3 F_avg = F0 + (1.0 - F0) / 21.0;
    //     float3 Fms = FssEss * F_avg / (1.0 - Ems * F_avg);
        
    //     float3 diffuseColor = (1-metalic) * albedo.xyz;
    //     float3 k_D = diffuseColor * (1.0 - FssEss - Fms*Ems);
        
    //     color += FssEss * radiance + (Fms * Ems + k_D) * irradiance;
    // }

    outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
}
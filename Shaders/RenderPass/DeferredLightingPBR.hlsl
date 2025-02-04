#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"
#include "Shadow/Shadow.hlsl"

ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);

float3 SingleScatteringIBL(float3 F0, float2 EnvBRDF, float3 diffuseColor, float3 radiance, float3 irradiance)
{
    float3 specularDFG = (F0 * EnvBRDF.x + EnvBRDF.yyy);
    
    float3 color = (specularDFG * radiance) + (diffuseColor * irradiance);
    return color;
}

#define WHITE_FURNACE_METHOD_OFF 0
#define WHITE_FURNACE_METHOD_SAMPLING 1
#define WHITE_FURNACE_METHOD_ALBEDO_ONLY

// A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, from Fdez-Aguera
// https://jcgt.org/published/0008/01/03/paper.pdf
float3 MultipleScatteringIBL(float roughness, float3 F0, float NoV, float2 EnvBRDF, float3 diffuseColor, float3 radiance, float3 irradiance)
{
    float3 splat = float3(1-roughness, 1-roughness, 1-roughness);
    float3 Fr = max(splat, F0) - F0;
    float3 k_S = F0 + Fr * pow(1.0 - NoV, 5.0);

    float3 FssEss = k_S * EnvBRDF.x + EnvBRDF.y;

    // Multiple scattering, from Fdez-Aguera
    float Ess = EnvBRDF.x + EnvBRDF.y;
    float Ems = (1.0 - Ess);
    float3 F_avg = F0 + (1.0 - F0) / 21.0;
    float3 FmsEms = Ems * FssEss * F_avg / (1.0 - Ems * F_avg);
    
    float3 k_D = diffuseColor * (1.0 - FssEss - FmsEms);
    
    float3 color = FssEss * radiance + (FmsEms + k_D) * irradiance;
    return color;
}

float3 WhiteFurnaceSampling(float3 V, float3 N, float roughness, float metalic, float3 diffuseColor, float3 F0, BxDFContext context, float3 energyCompensation)
{
    float3 color = float3(0,0,0);
    float3 diffuseTerm = diffuseLambert(diffuseColor);
    float lightIntensity = 1.f;

    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    uint numSample = 4096u;
    float invNumSample = 1.f/ numSample;
    
    for (uint i = 0u; i < numSample; ++i)
    {
        float2 Xi = Hammersley(i, invNumSample);

        // Specular : GGX importance sampling
        float3 H = ImportanceSampleGGX(Xi, roughness, N);
        float3 L = 2.0 * dot(V, H) * H - V;

        context.VoH = saturate(dot(V,H));
        context.NoH = saturate(dot(N,H));
        context.NoL = saturate(dot(N,L));

        if (context.NoL > 0)
        {
            float D = D_GGX(pow4(roughness), context.NoH);
            float pdf = D * context.NoH / (4.0 * context.VoH);
            
            float3 specularTerm = CookTorrenceSpecular(roughness, metalic, F0, context) * energyCompensation;
            float3 sampleColor = context.NoL * specularTerm;
            color += sampleColor / pdf;
        }
        
        // Diffuse : Uniform Sampling
        L = tangentToWorldCoords(UniformSampleHemisphere(Xi), N, s, t);
        context.NoL = saturate(dot(N, L));
        float pdf = 1 / (2*PI);
        
        if (context.NoL > 0)
        {
            float3 sampleColor = context.NoL * diffuseTerm;
            color += sampleColor / pdf;
        }
    }

    return color * (invNumSample * lightIntensity);
}

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
    if (renderResources.WhiteFurnaceMethod != WHITE_FURNACE_METHOD_OFF)
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
    context.NoV = saturate(dot(N,V)) + 1e-5; 

    float3 color = float3(0,0,0);

    float3 DIELECTRIC_SPECULAR = float3(0.04f, 0.04f, 0.04f);
    const float3 F0 = lerp(DIELECTRIC_SPECULAR, albedo.xyz, metalic);
    float3 diffuseColor = albedo.rgb * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalic);
    
    float4 EnvBRDF = EnvBRDFTexture.Sample(linearClampSampler, float2(context.NoV, roughness));
    float3 energyCompensation = 1.0 + F0 * (1.0 / EnvBRDF.z - 1.0);
    if (!renderResources.bUseEnergyCompensation)
    {
        energyCompensation = float3(1,1,1);
    }

    if (renderResources.WhiteFurnaceMethod == WHITE_FURNACE_METHOD_SAMPLING)
    {
        color += WhiteFurnaceSampling(V, N, roughness, metalic, diffuseColor, F0, context, energyCompensation);
        outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
        return;
    }

    {
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

            const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), sceneBuffer.inverseViewMatrix);
            const float4 lightSpacePosition = mul(worldSpacePosition, renderResources.lightViewProjectionMatrix);

            const float shadow = calculateShadow(lightSpacePosition, context.NoL, renderResources.shadowDepthTextureIndex);
            const float attenuation = (1-shadow);

            float3 diffuseTerm = diffuseLambert(diffuseColor);
            float3 specularTerm = CookTorrenceSpecular(roughness, metalic, F0, context) * energyCompensation;
            color += attenuation * lightColor.xyz * lightIntensity * context.NoL * (diffuseTerm + max(specularTerm, float3(0,0,0)));
        }
    }
    
    // Image Based Lighting
    float3 L = reflect(-V, N);
    float3 WorldSpaceL = normalize(mul(L, (float3x3)sceneBuffer.inverseViewMatrix));
    
    const float3 irradiance = IrradianceTexture.Sample(pointWrapSampler, worldSpaceNormal).rgb;
    uint PrefilterMipLevel = min(roughness * EnvMipCount, EnvMipCount-1);
    float3 radiance = PrefilterEnvmap.SampleLevel(pointClampSampler, WorldSpaceL, PrefilterMipLevel).rgb;

    if (renderResources.bUseEnvmap)
    {
        if (!renderResources.bUseEnergyCompensation)
        {
            color += SingleScatteringIBL(F0, EnvBRDF.xy, diffuseColor, radiance, irradiance);
        }
        else
        {
            color += MultipleScatteringIBL(roughness, F0, context.NoV, EnvBRDF.xy, diffuseColor, radiance, irradiance);
        }
    }

    outputTexture[dispatchThreadID.xy] = float4(color, 1.0f);
}
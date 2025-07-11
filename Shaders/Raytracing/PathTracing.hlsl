// clang-format off

#include "ShaderInterlop/RenderResources.hlsli"
#include "Raytracing/Common.hlsl"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"
#include "Shading/Sampling.hlsli"
#include "Raytracing/RaytracingUtils.hlsli"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space200);

ConstantBuffer<interlop::PathTraceRenderResources> renderResources : register(b0);

SamplerState MeshSampler : register(s0);
SamplerState LinearSampler : register(s1);

#define RANDOM_INDEX_PIXEL_JITTER 0
#define RANDOM_INDEX_GET_NEXT_RAY 1
#define MAX_FRAME_ACCUMULATED 3e4

#define PathTracingRayFlag RAY_FLAG_NONE

#define HITGROUP_OFFSET 0
#define HITGROUP_GEO_MULTIPLIER 2
#define MISSSHADER_IDX 0

[shader("raygeneration")] 
void RayGen()
{
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    RWTexture2D<uint> frameAccumulatedTexture = ResourceDescriptorHeap[renderResources.frameAccumulatedTextureIndex];
    const uint2 pixelCoord = DispatchRaysIndex().xy;
    
    uint frameAccumulated = frameAccumulatedTexture[pixelCoord];
    float3 prevRadiance = dstTexture[pixelCoord].xyz;
    if (renderResources.bRefreshPathTracingTexture)
    {
        dstTexture[pixelCoord] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        prevRadiance = float3(0.0f, 0.0f, 0.0f);
        frameAccumulatedTexture[pixelCoord] = 0;
        frameAccumulated = 0;
    }

    if (frameAccumulated > MAX_FRAME_ACCUMULATED)
    {
        return;
    }
    
    uint numSamples = renderResources.numSamples;
    float3 newRadiance = prevRadiance;

    for (uint s=0; s<numSamples; s++)
    {
        FPathTracePayload payload = defaultPathTracePayload(pixelCoord, s);

        int3 uvz = createUniqueUVZ(payload, renderResources.maxPathDepth);

        float4 randomFloat = renderResources.randomFloats[RANDOM_INDEX_PIXEL_JITTER];
        float du = pcgHash(randomFloat.x, uvz) - 0.5f;
        float dv = pcgHash(randomFloat.y, uvz) - 0.5f;

        // view ray generation
        float2 jitteredPixelCoord = float2(pixelCoord) + float2(du, dv);

        float2 ncdXY = (jitteredPixelCoord + 0.5f) / DispatchRaysDimensions().xy; // [0,1]
        ncdXY = ncdXY * 2.0f - 1.0f; // [-1,1]
        ncdXY.y = -ncdXY.y;

        RayDesc rayDesc = generateViewRay(renderResources.invViewProjectionMatrix, ncdXY);

        TraceRay(
            SceneBVH,
            PathTracingRayFlag,
            0xFF,
            HITGROUP_OFFSET,
            HITGROUP_GEO_MULTIPLIER,
            MISSSHADER_IDX,
            rayDesc,
            payload
        );

        float3 currRadiance = payload.radiance;
        const float lerpFactor = frameAccumulated / float(frameAccumulated + 1.f);
        newRadiance = lerp(currRadiance, newRadiance, lerpFactor);

        frameAccumulated += 1;
    }

    dstTexture[pixelCoord] = float4(newRadiance, 1.0f);
    frameAccumulatedTexture[pixelCoord] = frameAccumulated;
}

void getDiffuseSpecularRefractionFactor(inout float diffuseFactor, inout float specularFactor, inout float refractionFactor)
{
    ConstantBuffer<interlop::DebugBuffer> debugBuffer = ResourceDescriptorHeap[renderResources.debugBufferIndex];
    
    if (debugBuffer.bEnableDiffuse) 
    {
        if (debugBuffer.bEnableSpecular)
        {
            diffuseFactor = 0.5f; specularFactor = 0.5f;
        }
        else
        {
            diffuseFactor = 1.0f;
        }
    }
    else
    {
        if (debugBuffer.bEnableSpecular)
        {
            specularFactor = 0.5f; 
        }
    }
    
    float notRefractionFactor = 1.0f - refractionFactor;
    diffuseFactor *= notRefractionFactor;
    specularFactor *= notRefractionFactor;
}

float3 sampleNextRay(float3 inRayTS, float3 normalTS, int3 uvz, float3 albedo, float metallic, float roughness, float3 F0, float3 energyCompensation, float refractionFactor, float IOR, out float3 throughput)
{
    float diffuseFactor = 0.0f;
    float specularFactor = 0.0f;
    getDiffuseSpecularRefractionFactor(diffuseFactor, specularFactor, refractionFactor);

    float4 randomFloat = renderResources.randomFloats[RANDOM_INDEX_GET_NEXT_RAY];
    float rx = pcgHash(randomFloat.x, uvz);
    float ry = pcgHash(randomFloat.y, uvz);
    float rz = pcgHash(randomFloat.z, uvz);
    float rw = pcgHash(randomFloat.w, uvz);

    if (rx < diffuseFactor)
    {
        // diffuse ray
        float2 u = float2(ry, rz);
        float pdf;
        float NoL;
        float3 d;

        // The PDF of sampling a cosine hemisphere is NdotL / Pi, which cancels out those terms
        // from the diffuse BRDF and the irradiance integral
        ImportanceSampleCosDir(u, normalTS, d, NoL, pdf);

        float3 V = -inRayTS;
        float3 H = normalize(V + d);
        float VoH = saturate(dot(V, H));
        float3 diffuse = lambertianDiffuseBRDF(albedo, VoH, metallic);
        throughput = diffuse * NoL / (pdf * diffuseFactor);
        return d;
    }
    else if (rx < diffuseFactor + specularFactor)
    {
        float3 outRayDir = ImportanceSampleGGX_V2(-inRayTS, normalTS, roughness, ry, rz, F0, energyCompensation, throughput);
        throughput = max(throughput / specularFactor, float3(0.0f, 0.0f, 0.0f));
        return outRayDir;
    }
    else if (rx < diffuseFactor + specularFactor + refractionFactor)
    {
        // refraction ray
        bool bFrontFace = dot(inRayTS, normalTS) < 0.0f;
        float3 actualNormal = bFrontFace ? normalTS : -normalTS;
        float eta = bFrontFace ? 1.0f / IOR : IOR;
        float3 outRayDir = refract(inRayTS, actualNormal, eta);
        if (length(outRayDir) < 0.001f)
        {
            // fallback to reflection
            outRayDir = reflect(inRayTS, actualNormal);
        }

        throughput = max(albedo / refractionFactor, float3(0.0f, 0.0f, 0.0f));
        return outRayDir;
    }

    throughput = float3(0.0f, 0.0f, 0.0f);
    return float3(0.0f, 0.0f, 1.0f); // invalid ray direction
}

[shader("closesthit")] 
void ClosestHit(inout FPathTracePayload payload, in Attributes attr) 
{
    int3 uvz = createUniqueUVZ(payload, renderResources.maxPathDepth);

    BufferIndexContext context = {
        renderResources.geometryInfoBufferIdx,
        renderResources.materialBufferIdx
    };

    const interlop::MeshVertex hitSurface = GetHitSurface(attr, context, InstanceID());
    const interlop::FRaytracingMaterial material = GetGeometryMaterial(context, InstanceID());

    float2 textureCoords = hitSurface.texcoord;
    float4 albedoEmissive = getAlbedoSample(textureCoords, material.albedoTextureIndex, MeshSampler, material.albedoColor);
    float3 albedo = albedoEmissive.xyz;

    // Russian Roulette
    float RandomFloatForRoulette = pcgHash(renderResources.randomFloats[RANDOM_INDEX_PIXEL_JITTER].z, uvz);
    bool bTerminate = TerminateByRussianRoulette(RandomFloatForRoulette, albedo);
    if (bTerminate)
    {
        payload.radiance = float3(0.f, 0.f, 0.f);
        payload.distance = 0;
        return;
    }

    float3 orm = getOcclusionRoughnessMetallicSample(textureCoords, float2(material.metallic, material.roughness),
        material.ormTextureIndex, material.metalRoughnessTextureIndex, -1, MeshSampler);
        
    float metallic = orm.z;
    float roughness = orm.y;
    float3 emissive = material.emissiveColor.xyz;

    if (emissive.x > 0.f)
    {
        payload.radiance = emissive;
        payload.distance = 0;
        return;
    }
    
    const float3 DIELECTRIC_SPECULAR = float3(0.04f, 0.04f, 0.04f);
    const float3 F0 = lerp(DIELECTRIC_SPECULAR, albedo.xyz, metallic);
    
    Texture2D<float4> EnvBRDFTexture = ResourceDescriptorHeap[renderResources.envBRDFTextureIndex];
    
    float4x3 OTW4x3 = ObjectToWorld4x3();
    float3x3 ObjectToWorld3x3 = float3x3(
        OTW4x3[0].xyz,
        OTW4x3[1].xyz,
        OTW4x3[2].xyz
    );

    float3x3 ObjectToWorld = transpose(Inverse3x3(ObjectToWorld3x3));

    float3 normalWS = normalize(mul(hitSurface.normal, ObjectToWorld).xyz);
    float3 tangentWS = normalize(mul(hitSurface.tangent, ObjectToWorld).xyz);
    const float3 biTangentWS = normalize(cross(normalWS, tangentWS));

    float3x3 tangentToWorld = float3x3(tangentWS, biTangentWS, normalWS);

    float3x3 worldToTangent = Inverse3x3(tangentToWorld);
    
    float3 normalTS = getNormalSampleTS(textureCoords, material.normalTextureIndex, MeshSampler);

    // next ray
    float3 inRay = WorldRayDirection();
    float3 inRayTS = normalize(mul(inRay, worldToTangent));
    
    // energy compensation
    float NoV = saturate(dot(normalTS, -inRayTS));
    NoV = min(NoV, 0.9f); // TODO: temp
    float4 EnvBRDF = EnvBRDFTexture.SampleLevel(MeshSampler, float2(NoV, roughness), 0);
    float3 energyCompensation = 1.0 + F0 * (1.0 / EnvBRDF.z - 1.0);

    float3 throughput = float3(0,0,0);
    float3 nextRay = sampleNextRay(inRayTS, normalTS, uvz, albedo, metallic, roughness, F0, energyCompensation, material.refractionFactor, material.IOR, throughput);

    float3 nextRayWS = normalize(mul(nextRay, tangentToWorld));
    
    const float3 positionWS = mul(float4(hitSurface.position,1), OTW4x3).xyz;
    RayDesc ray;
    ray.Origin = positionWS;
    ray.Direction = nextRayWS;
    ray.TMin = RAY_MIN;
    ray.TMax = FP32Max;

    FPathTracePayload nextPayload;
    nextPayload.radiance = 0.0f;
    nextPayload.distance = 0.0f;
    nextPayload.depth = payload.depth+1;
    nextPayload.uv = payload.uv;
    nextPayload.sampleIndex = payload.sampleIndex;
    
    if (payload.depth+1 >= renderResources.maxPathDepth)
    {
        payload.radiance = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    TraceRay(
        SceneBVH,
        PathTracingRayFlag,
        0xFFFFFFFF, 
        HITGROUP_OFFSET,
        HITGROUP_GEO_MULTIPLIER,
        MISSSHADER_IDX,
        ray,
        nextPayload);

    payload.radiance = throughput * nextPayload.radiance;
    payload.distance = 0;
}

[shader("miss")]
void Miss(inout FPathTracePayload payload : SV_RayPayload)
{
    TextureCube<float4> EnvMapTexture = ResourceDescriptorHeap[renderResources.envmapTextureIndex];
    float3 rayDir = WorldRayDirection();
    float3 envRadiance = EnvMapTexture.SampleLevel(MeshSampler, rayDir, 0).rgb;
    
    payload.radiance = envRadiance * renderResources.envmapIntensity;
    payload.distance = -1;

    // Directional Light
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];

    uint DLIndex = 0;
    float4 lightColor = lightBuffer.lightColor[DLIndex];
    float lightIntensity = lightBuffer.intensityDistance[DLIndex][0];
    float3 lightDir = normalize(-lightBuffer.lightPosition[DLIndex].xyz);
    if (dot(rayDir, lightDir) > 0.99f) // ray is facing the directional light.
    {
        payload.radiance += lightColor.xyz * lightIntensity;
    }
}

[shader("anyhit")]
void AnyHit(inout FPathTracePayload payload, in Attributes attr)
{
    if (payload.depth >= renderResources.maxPathDepth)
    {
        payload.radiance = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    int3 uvz = createUniqueUVZ(payload, renderResources.maxPathDepth);

    BufferIndexContext context = {
        renderResources.geometryInfoBufferIdx,
        renderResources.materialBufferIdx
    };

    const interlop::MeshVertex hitSurface = GetHitSurface(attr, context, InstanceID());
    const interlop::FRaytracingMaterial material = GetGeometryMaterial(context, InstanceID());

    float2 textureCoords = hitSurface.texcoord;
    float4 albedoEmissive = getAlbedoSample(textureCoords, material.albedoTextureIndex, MeshSampler, material.albedoColor);
    float alpha = albedoEmissive.a;

    static const float AlphaThreshold = 0.9f;
    if (alpha < AlphaThreshold)
    {
        IgnoreHit();
    }
}
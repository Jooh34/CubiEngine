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

#define PathTracingRayFlag RAY_FLAG_CULL_BACK_FACING_TRIANGLES

[shader("raygeneration")] 
void RayGen()
{
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    RWTexture2D<uint> frameAccumulatedTexture = ResourceDescriptorHeap[renderResources.frameAccumulatedTextureIndex];

    const uint2 pixelCoord = DispatchRaysIndex().xy;
    
    uint traceRayFlags = PathTracingRayFlag;

    const uint hitGroupOffset = 0;
    const uint hitGroupGeoMultiplier = 2;
    const uint missShaderIdx = 0;

    uint numSamples = renderResources.numSamples;
    float3 radianceSum = float3(0.0f, 0.0f, 0.0f);

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
            traceRayFlags,
            0xFF,
            hitGroupOffset,
            hitGroupGeoMultiplier,
            missShaderIdx,
            rayDesc,
            payload
        );

        radianceSum += payload.radiance;
    }

    uint frameAccumulated = 0;
    if (renderResources.bRefreshPathTracingTexture)
    {
        // reset
        dstTexture[pixelCoord] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        frameAccumulatedTexture[pixelCoord] = 0;
    }
    else
    {
        frameAccumulated = frameAccumulatedTexture[pixelCoord];
    }

    int newFrameAccumulated = frameAccumulated + numSamples;

    float3 result = (dstTexture[pixelCoord].xyz * frameAccumulated + radianceSum) / newFrameAccumulated;

    dstTexture[pixelCoord] = float4(result, 1.0f);
    frameAccumulatedTexture[pixelCoord] = newFrameAccumulated;
}

float3 sampleNextRay_oldver(float3 inRay, float3 N, int3 uvz, float3 albedo, float3 F0, float roughness, out float3 throughput)
{
    float reflectionFactor = 1.0f - roughness;

    float4 randomFloat = renderResources.randomFloats[RANDOM_INDEX_GET_NEXT_RAY];
    float rx = pcgHash(randomFloat.x, uvz);
    float ry = pcgHash(randomFloat.y, uvz);
    float rz = pcgHash(randomFloat.z, uvz);
    float rw = pcgHash(randomFloat.w, uvz);

    if (rx < reflectionFactor) // reflect ray
    {
        float3 reflected_ray = inRay - N * (2 * dot(N, inRay));
        float e1 = ry - 0.5;
        float e2 = rz - 0.5;
        float e3 = rw - 0.5;
        float3 rand_vec = float3(e1 * roughness, e2 * roughness, e3 * roughness);

        throughput = F0;
        return normalize(reflected_ray + rand_vec);
    }
    else
    {
        // diffuse ray
        float2 u = float2(ry, rz);
        float pdf;
        float NoL;
        float3 d;

        // The PDF of sampling a cosine hemisphere is NdotL / Pi, which cancels out those terms
        ImportanceSampleCosDir(u, N, d, NoL, pdf);

        throughput = albedo;
        return d;
    }
}

void getDebugDiffuseSpecularFactor(inout float DiffuseFactor, inout float SpecularFactor)
{
    ConstantBuffer<interlop::DebugBuffer> debugBuffer = ResourceDescriptorHeap[renderResources.debugBufferIndex];
    
    if (debugBuffer.bEnableDiffuse) 
    {
        if (debugBuffer.bEnableSpecular)
        {
            DiffuseFactor = 0.5f; SpecularFactor = 0.5f;
        }
        else
        {
            DiffuseFactor = 1.0f;
        }
    }
    else
    {
        if (debugBuffer.bEnableSpecular)
        {
            SpecularFactor = 0.5f; 
        }
    }
}

float3 sampleNextRay(float3 inRayTS, float3 normalTS, int3 uvz, float3 albedo, float metallic, float roughness, float3 F0, float3 energyCompensation, out float3 throughput)
{
    float DiffuseFactor = 0.0f;
    float SpecularFactor = 0.0f;
    getDebugDiffuseSpecularFactor(DiffuseFactor, SpecularFactor);

    float4 randomFloat = renderResources.randomFloats[RANDOM_INDEX_GET_NEXT_RAY];
    float rx = pcgHash(randomFloat.x, uvz);
    float ry = pcgHash(randomFloat.y, uvz);
    float rz = pcgHash(randomFloat.z, uvz);
    float rw = pcgHash(randomFloat.w, uvz);

    if (dot(normalTS, -inRayTS) < 0.0f)
    {
        // If the incoming ray is below the surface, we discard it.
        throughput = float3(0.0f, 0.0f, 0.0f);
        return float3(0.0f, 0.0f, 1.0f);
    }

    if (rx < DiffuseFactor)
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
        throughput = diffuse * NoL / (pdf * DiffuseFactor);
        return d;
    }
    else if (rx < DiffuseFactor + SpecularFactor)
    {
        float3 outRayDir = ImportanceSampleGGX_V3(-inRayTS, normalTS, roughness, ry, rz, F0, energyCompensation, throughput);
        throughput = throughput / SpecularFactor;
        return outRayDir;
    }

    throughput = float3(0.0f, 0.0f, 0.0f);
    return float3(0.0f, 0.0f, 1.0f); // invalid ray direction
}

[shader("closesthit")] 
void ClosestHit(inout FPathTracePayload payload, in Attributes attr) 
{
    if (payload.depth >= renderResources.maxPathDepth)
    {
        payload.radiance = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    int3 uvz = createUniqueUVZ(payload, renderResources.maxPathDepth);

    BufferIndexContext context = {
        renderResources.geometryInfoBufferIdx,
        renderResources.vtxBufferIdx,
        renderResources.idxBufferIdx,
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

    float2 metallicRoughness = getMetallicRoughnessSample(textureCoords, material.metalRoughnessTextureIndex, MeshSampler, float2(material.metallic, material.roughness));
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;
    float3 emissive = material.emissiveColor.xyz;
    if (emissive.x > 0.f)
    {
        float dist = RayTCurrent();
        float scalefactor = 1.0e+3;
        float attenuation = scalefactor / (dist * dist);
        payload.radiance = emissive * attenuation;
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

    float3x3 tangentToWorld;
    float3 normalWS = normalize(mul(hitSurface.normal, ObjectToWorld).xyz);
    float3 tangentWS = normalize(mul(hitSurface.tangent, ObjectToWorld).xyz);
    const float3 biTangentWS = normalize(cross(normalWS, tangentWS));

    tangentToWorld = float3x3(tangentWS, biTangentWS, normalWS);
    float3x3 worldToTangent = transpose(tangentToWorld);
    
    float3 normalTS = getNormalSampleTS(textureCoords, material.normalTextureIndex, MeshSampler);

    // next ray
    float3 inRay = WorldRayDirection();
    float3 inRayTS = normalize(mul(inRay, worldToTangent));
    
    // energy compensation
    float NoV = saturate(dot(normalTS, -inRayTS));
    NoV = min(NoV, 0.9f); // TODO: temp
    float4 EnvBRDF = EnvBRDFTexture.SampleLevel(MeshSampler, float2(NoV, roughness), 0);
    float3 energyCompensation = 1.0 + F0 * (1.0 / EnvBRDF.z - 1.0);

    float3 throughput = albedo;
    // float3 sampleNextRay(float3 inRayTS, float3 normalTS, int3 uvz, float3 albedo, float metallic, float roughness, float3 F0, float energyCompensation, out float3 throughput)
    // float3 nextRay = sampleNextRay(inRayTS, normalTS, uvz, albedo, metallic, roughness, F0, energyCompensation, throughput);
    float3 nextRay = sampleNextRay_oldver(inRayTS, normalTS, uvz, albedo, F0, roughness, throughput);

    float3 nextRayWS = normalize(mul(nextRay, tangentToWorld));
    
    const float3 positionWS = mul(float4(hitSurface.position,1), OTW4x3).xyz;
    RayDesc ray;
    ray.Origin = positionWS;
    ray.Direction = nextRayWS;
    ray.TMin = 0.1f;
    ray.TMax = FP32Max;

    FPathTracePayload nextPayload;
    nextPayload.radiance = 0.0f;
    nextPayload.distance = 0.0f;
    nextPayload.depth = payload.depth+1;
    nextPayload.uv = payload.uv;
    nextPayload.sampleIndex = payload.sampleIndex;

    uint traceRayFlags = PathTracingRayFlag;
    
    const uint hitGroupOffset = RayTypeRadiance;
    const uint hitGroupGeoMultiplier = NumRayTypes;
    const uint missShaderIdx = RayTypeRadiance;

    TraceRay(SceneBVH, traceRayFlags, 0xFFFFFFFF, hitGroupOffset, hitGroupGeoMultiplier, missShaderIdx, ray, nextPayload);

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
        renderResources.vtxBufferIdx,
        renderResources.idxBufferIdx,
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
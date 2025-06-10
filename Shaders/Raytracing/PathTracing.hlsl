// clang-format off

#include "ShaderInterlop/RenderResources.hlsli"
#include "Raytracing/Common.hlsl"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space200);

ConstantBuffer<interlop::PathTraceRenderResources> renderResources : register(b0);

SamplerState MeshSampler : register(s0);
SamplerState LinearSampler : register(s1);

#define RANDOM_INDEX_GET_NEXT_RAY 0

[shader("raygeneration")] 
void RayGen()
{
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    RWTexture2D<uint> frameAccumulatedTexture = ResourceDescriptorHeap[renderResources.frameAccumulatedTextureIndex];
    const uint2 pixelCoord = DispatchRaysIndex().xy;

    float2 ncdXY = (float2(pixelCoord) + 0.5f) / DispatchRaysDimensions().xy; // [0,1]
    ncdXY = ncdXY * 2.0f - 1.0f; // [-1,1]
    ncdXY.y = -ncdXY.y;
    
    float4 rayStart = mul(float4(ncdXY, 1.0f, 1.0f), renderResources.invViewProjectionMatrix);
    float4 rayEnd = mul(float4(ncdXY, 0.0f, 1.0f), renderResources.invViewProjectionMatrix);

    rayStart.xyz /= rayStart.w;
    rayEnd.xyz /= rayEnd.w;
    float3 rayDir = normalize(rayEnd.xyz - rayStart.xyz);
    float rayLength = length(rayEnd.xyz - rayStart.xyz);
    
    // Trace a primary ray
    RayDesc ray;
    ray.Origin = rayStart.xyz;
    ray.Direction = rayDir;
    ray.TMin = 0.01f;
    ray.TMax = rayLength;

    FPathTracePayload payload;
    payload.radiance = 0.0f;
    payload.distance = 0.0f;
    payload.depth = 0;
    payload.uv = pixelCoord;
    
    uint traceRayFlags = RAY_FLAG_NONE;

    const uint hitGroupOffset = 0;
    const uint hitGroupGeoMultiplier = 2;
    const uint missShaderIdx = 0;

    int numSamples = 1;
    float3 radianceSum = float3(0.0f, 0.0f, 0.0f);
    for (int s=0; s<numSamples; s++)
    {
        TraceRay(
            SceneBVH,
            traceRayFlags,

            // Parameter name: InstanceInclusionMask
            // Instance inclusion mask, which can be used to mask out some geometry to this ray by
            // and-ing the mask with a geometry mask. The 0xFF flag then indicates no geometry will be
            // masked
            0xFF,

            // Parameter name: RayContributionToHitGroupIndex
            // Depending on the type of ray, a given object can have several hit groups attached
            // (ie. what to do when hitting to compute regular shading, and what to do when hitting
            // to compute shadows). Those hit groups are specified sequentially in the SBT, so the value
            // below indicates which offset (on 4 bits) to apply to the hit groups for this ray. In this
            // sample we only have one hit group per object, hence an offset of 0.
            hitGroupOffset,

            // Parameter name: MultiplierForGeometryContributionToHitGroupIndex
            // The offsets in the SBT can be computed from the object ID, its instance ID, but also simply
            // by the order the objects have been pushed in the acceleration structure. This allows the
            // application to group shaders in the SBT in the same order as they are added in the AS, in
            // which case the value below represents the stride (4 bits representing the number of hit
            // groups) between two consecutive objects.
            hitGroupGeoMultiplier,

            // Parameter name: MissShaderIndex
            // Index of the miss shader to use in case several consecutive miss shaders are present in the
            // SBT. This allows to change the behavior of the program when no geometry have been hit, for
            // example one to return a sky color for regular rendering, and another returning a full
            // visibility value for shadow rays. This sample has only one miss shader, hence an index 0
            missShaderIdx,

            ray,
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

    dstTexture[pixelCoord] = dstTexture[pixelCoord] * frameAccumulated / newFrameAccumulated + float4(radianceSum, 1.0f) / newFrameAccumulated;
    frameAccumulatedTexture[pixelCoord] = frameAccumulated + numSamples;
}

float hashIntFloatCombo(float baseRand, int3 uvz)
{
    float3 p = float3(uvz) + baseRand * float3(1.0f, 17.0f, 113.0f); // mix scaling
    return frac(sin(dot(p, float3(12.9898, 78.233, 37.719))) * 43758.5453);
}

float pcgHash(float baseRand, int3 p)
{
    uint state = uint(p.x) * 747796405u + uint(p.y) * 2891336453u + uint(p.z) * 11863279u;
    state ^= uint(baseRand * 1234567.0f); // inject baseRand entropy
    state ^= (state >> 17);
    state *= 0xed5ad4bb;
    state ^= (state >> 11);
    return float(state & 0x00FFFFFFu) / float(0x01000000); // map to [0,1)
}

float3 getNextRay(float3 inRay, float3 N, float roughness, int3 uvz)
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

        return normalize(reflected_ray + rand_vec);
    }
    else
    {
        // diffuse ray
        float3 nl = dot(N, inRay) < 0 ? N : -N;
        float r1 = 2 * PI * ry;
        float r2 = rz;
        float r2s = sqrt(r2);

        float3 w = nl;
        float3 u = normalize(cross((abs(w.x) > 0.1f ? float3(0,1,0) : float3(1,1,1)), w));
        float3 v=cross(w, u);
        
        float3 d = normalize(u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1-r2));

        return d;
    }
}

[shader("closesthit")] 
void ClosestHit(inout FPathTracePayload payload, in Attributes attr) 
{
    if (payload.depth >= renderResources.maxPathDepth)
    {
        payload.radiance = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    int3 uvz = int3(payload.uv, payload.depth);

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
    
    payload.radiance = albedo;
    
    float3x3 tangentToWorld = float3x3(hitSurface.tangent, hitSurface.bitangent, hitSurface.normal);
    float3 N = getNormalSample(textureCoords, material.normalTextureIndex, MeshSampler, float3(0,0,1), tangentToWorld).xyz;

    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[material.metalRoughnessTextureIndex];
    float2 metalRoughness = metalRoughnessTexture.SampleLevel(MeshSampler, textureCoords, 0.f).bg;
    
    float metalic = metalRoughness.x;
    float roughness = metalRoughness.y;
    
    const float3 positionWS = hitSurface.position;

    // next ray
    float3 inRay = WorldRayDirection();
    float3 nextRay = getNextRay(inRay, N, roughness, uvz);
    
    // next ray
    RayDesc ray;
    ray.Origin = positionWS;
    ray.Direction = nextRay;
    ray.TMin = 0.1f;
    ray.TMax = FP32Max;

    FPathTracePayload nextPayload;
    nextPayload.radiance = 0.0f;
    nextPayload.distance = 0.0f;
    nextPayload.depth = payload.depth+1;
    nextPayload.uv = payload.uv;

    uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
    
    const uint hitGroupOffset = RayTypeRadiance;
    const uint hitGroupGeoMultiplier = NumRayTypes;
    const uint missShaderIdx = RayTypeRadiance;
    TraceRay(SceneBVH, traceRayFlags, 0xFFFFFFFF, hitGroupOffset, hitGroupGeoMultiplier, missShaderIdx, ray, nextPayload);

    payload.radiance = albedo * nextPayload.radiance;
    payload.distance = 0;
}

[shader("miss")]
void Miss(inout FPayload payload : SV_RayPayload)
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
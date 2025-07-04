// clang-format off

#include "ShaderInterlop/RenderResources.hlsli"
#include "Raytracing/Common.hlsl"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space200);

ConstantBuffer<interlop::RTSceneDebugRenderResource> renderResources : register(b0);

SamplerState MeshSampler : register(s0);
SamplerState LinearSampler : register(s1);

#define TraceRayFlag RAY_FLAG_CULL_BACK_FACING_TRIANGLES

[shader("raygeneration")] 
void RayGen()
{
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
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
    ray.TMin = RAY_MIN;
    ray.TMax = rayLength;

    FPayload payload;
    payload.radiance = 0.0f;
    payload.distance = 0.0f;
    
    const uint hitGroupOffset = RayTypeRadiance;
    const uint hitGroupGeoMultiplier = NumRayTypes;
    const uint missShaderIdx = RayTypeRadiance;

    TraceRay(
        SceneBVH,
        TraceRayFlag,
        0xFF,
        hitGroupOffset,
        hitGroupGeoMultiplier,
        missShaderIdx,
        ray,
        payload
    );

    dstTexture[pixelCoord] = float4(payload.radiance, 1.f);
}

[shader("closesthit")] 
void ClosestHit(inout FPayload payload, in Attributes attr) 
{
    float3 color = float3(0.0f, 0.0f, 0.0f);

    BufferIndexContext context = {
        renderResources.geometryInfoBufferIdx,
        renderResources.materialBufferIdx
    };
    
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    const interlop::MeshVertex hitSurface = GetHitSurface(attr, context, InstanceID());
    const interlop::FRaytracingMaterial material = GetGeometryMaterial(context, InstanceID());

    float2 textureCoords = hitSurface.texcoord;
    float4 albedoEmissive = getAlbedoSample(textureCoords, material.albedoTextureIndex, MeshSampler, material.albedoColor);
    float3 albedo = albedoEmissive.xyz;
    
    float4x3 OTW4x3 = ObjectToWorld4x3();
    float3x3 ObjectToWorld3x3 = float3x3(
        OTW4x3[0].xyz,
        OTW4x3[1].xyz,
        OTW4x3[2].xyz
    );

    float3x3 ObjectToWorld = transpose(Inverse3x3(ObjectToWorld3x3));

    const float3 normalWS = normalize(mul(hitSurface.normal, ObjectToWorld).xyz);
    const float3 tangentWS = normalize(mul(hitSurface.tangent, ObjectToWorld).xyz);
    const float3 biTangentWS = normalize(cross(normalWS, tangentWS));
    
    float3x3 tangentToWorld = float3x3(tangentWS, biTangentWS, normalWS);

    float3 N = getNormalSample(textureCoords, material.normalTextureIndex, MeshSampler, normalWS, tangentToWorld).xyz;
    float3x3 viewMatrix = (float3x3)transpose(sceneBuffer.inverseViewMatrix);
    N = mul(N, viewMatrix);

    float3 orm = getOcclusionRoughnessMetallicSample(textureCoords, float2(material.metallic, material.roughness),
        material.ormTextureIndex, material.metalRoughnessTextureIndex, -1, MeshSampler);
    
    float roughness = orm.y;
    float metalic = orm.z;

    const float3 positionWS = hitSurface.position;

    // {
    //     // Directional Light
    //     uint DLIndex = 0;
    //     float4 lightColor = lightBuffer.lightColor[DLIndex];
    //     float lightIntensity = lightBuffer.intensityDistance[DLIndex][0];

    //     const float3 viewSpacePosition = mul(float4(positionWS, 1.0f), sceneBuffer.viewMatrix).xyz;
    //     const float3 V = normalize(-viewSpacePosition);
        
    //     BxDFContext context = (BxDFContext)0;
    //     context.NoV = saturate(dot(N,V)); 
        
    //     float3 DIELECTRIC_SPECULAR = float3(0.04f, 0.04f, 0.04f);
    //     const float3 F0 = lerp(DIELECTRIC_SPECULAR, albedo.xyz, metalic);
    //     float3 diffuseColor = albedo.rgb * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalic);

    //     if (lightIntensity > 0.f)
    //     {
    //         float3 directionalResult = float3(0.f, 0.f, 0.f);
    //         const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[DLIndex].xyz);
    //         const float3 H = normalize(V+L);

    //         context.VoH = saturate(dot(V,H));
    //         context.NoH = saturate(dot(N,H));
    //         context.NoL = saturate(dot(N,L));

    //         const float4 worldspaceL_ = mul(float4(L, 0.0f), sceneBuffer.inverseViewMatrix);
    //         const float3 worldspaceL = normalize(worldspaceL_.xyz);
            
    //         float3 diffuseTerm = lambertianDiffuseBRDF(albedo.xyz, context.VoH, metalic);
    //         float energyCompensation = 1.f;
    //         float3 specularTerm = CookTorrenceSpecular(roughness, metalic, F0, context) * energyCompensation;
            
    //         directionalResult += lightColor.xyz * lightIntensity * context.NoL * (diffuseTerm + max(specularTerm, float3(0,0,0)));

    //         // shadow ray
    //         RayDesc ray;
    //         ray.Origin = positionWS;
    //         ray.Direction = worldspaceL;
    //         ray.TMin = RAY_MIN;
    //         ray.TMax = FP32Max;

    //         FShadowPayload shadowPayload;
    //         shadowPayload.visibility = 1.0f;

    //         uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
            
    //         const uint hitGroupOffset = RayTypeShadow;
    //         const uint hitGroupGeoMultiplier = NumRayTypes;
    //         const uint missShaderIdx = RayTypeShadow;
    //         TraceRay(SceneBVH, traceRayFlags, 0xFFFFFFFF, hitGroupOffset, hitGroupGeoMultiplier, missShaderIdx, ray, shadowPayload);

    //         directionalResult *= shadowPayload.visibility;
    //         color += directionalResult;
    //     }
    // }

    payload.radiance = albedo;
    payload.distance = (InstanceID()/103.f);
}

[shader("miss")]
void Miss(inout FPayload payload : SV_RayPayload)
{
    TextureCube<float4> EnvMapTexture = ResourceDescriptorHeap[renderResources.envmapTextureIndex];
    float3 rayDir = WorldRayDirection();
    float3 envRadiance = EnvMapTexture.SampleLevel(MeshSampler, rayDir, 0).rgb;

    payload.radiance = envRadiance;
    payload.distance = -1;
}
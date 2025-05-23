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
    ray.TMin = 0.0f;
    ray.TMax = rayLength;

    FPayload payload;
    payload.radiance = 0.0f;
    payload.distance = 0.0f;
    
    uint traceRayFlags = RAY_FLAG_NONE;

    const uint hitGroupOffset = RayTypeRadiance;
    const uint hitGroupGeoMultiplier = NumRayTypes;
    const uint missShaderIdx = RayTypeRadiance;

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

    dstTexture[pixelCoord] = float4(payload.radiance, 1.f);
}

[shader("closesthit")] 
void ClosestHit(inout FPayload payload, in Attributes attr) 
{
    float3 color = float3(0.0f, 0.0f, 0.0f);

    const interlop::MeshVertex hitSurface = GetHitSurface(attr, renderResources, InstanceID());
    const interlop::FRaytracingMaterial material = GetGeometryMaterial(renderResources, InstanceID());

    float2 textureCoords = hitSurface.texcoord;
    float4 albedoEmissive = getAlbedoSample(textureCoords, material.albedoTextureIndex, MeshSampler, material.albedoColor);
    float3 albedo = albedoEmissive.xyz;
    
    float3x3 tangentToWorld = float3x3(hitSurface.tangent, hitSurface.bitangent, hitSurface.normal);
    float3 N = getNormalSample(textureCoords, material.normalTextureIndex, MeshSampler, float3(0,0,1), tangentToWorld).xyz;

    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[material.metalRoughnessTextureIndex];
    float2 metalRoughness = metalRoughnessTexture.SampleLevel(MeshSampler, textureCoords, 0.f).bg;
    
    float metalic = metalRoughness.x;
    float roughness = metalRoughness.y;

    const float3 positionWS = hitSurface.position;

    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    {
        // Directional Light
        uint DLIndex = 0;
        float4 lightColor = lightBuffer.lightColor[DLIndex];
        float lightIntensity = lightBuffer.intensityDistance[DLIndex][0];

        const float3 viewSpacePosition = mul(float4(positionWS, 1.0f), sceneBuffer.viewMatrix).xyz;
        const float3 V = normalize(-viewSpacePosition);
        
        BxDFContext context = (BxDFContext)0;
        context.NoV = saturate(dot(N,V)); 
        
        float3 DIELECTRIC_SPECULAR = float3(0.04f, 0.04f, 0.04f);
        const float3 F0 = lerp(DIELECTRIC_SPECULAR, albedo.xyz, metalic);
        float3 diffuseColor = albedo.rgb * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalic);

        if (lightIntensity > 0.f)
        {
            float3 directionalResult = float3(0.f, 0.f, 0.f);
            const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[DLIndex].xyz);
            const float3 H = normalize(V+L);

            context.VoH = saturate(dot(V,H));
            context.NoH = saturate(dot(N,H));
            context.NoL = saturate(dot(N,L));

            const float4 worldspaceL_ = mul(float4(L, 0.0f), sceneBuffer.inverseViewMatrix);
            const float3 worldspaceL = normalize(worldspaceL_.xyz);
            
            float3 diffuseTerm = lambertianDiffuseBRDF(albedo.xyz, context.VoH, metalic);
            float energyCompensation = 1.f;
            float3 specularTerm = CookTorrenceSpecular(roughness, metalic, F0, context) * energyCompensation;
            
            directionalResult += lightColor.xyz * lightIntensity * context.NoL * (diffuseTerm + max(specularTerm, float3(0,0,0)));

            // shadow ray
            RayDesc ray;
            ray.Origin = positionWS;
            ray.Direction = worldspaceL;
            ray.TMin = 0.001f;
            ray.TMax = FP32Max;

            FShadowPayload shadowPayload;
            shadowPayload.visibility = 1.0f;

            uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
            
            const uint hitGroupOffset = RayTypeShadow;
            const uint hitGroupGeoMultiplier = NumRayTypes;
            const uint missShaderIdx = RayTypeShadow;
            TraceRay(SceneBVH, traceRayFlags, 0xFFFFFFFF, hitGroupOffset, hitGroupGeoMultiplier, missShaderIdx, ray, shadowPayload);

            directionalResult *= shadowPayload.visibility;
            color += directionalResult;
        }
    }

    payload.radiance = color;
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
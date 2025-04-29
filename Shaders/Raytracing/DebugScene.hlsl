// clang-format off

#include "ShaderInterlop/RenderResources.hlsli"
#include "Raytracing/Common.hlsl"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space200);
// RWTexture2D<float4> RenderTarget : register(u0);

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
    
    // uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

    // const uint hitGroupOffset = RayTypeShadow;
    // const uint hitGroupGeoMultiplier = NumRayTypes;
    // const uint missShaderIdx = RayTypeShadow;

    TraceRay(
      SceneBVH,
      // Parameter name: RayFlags
      // Flags can be used to specify the behavior upon hitting a surface
      RAY_FLAG_NONE,

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
      0,

      // Parameter name: MultiplierForGeometryContributionToHitGroupIndex
      // The offsets in the SBT can be computed from the object ID, its instance ID, but also simply
      // by the order the objects have been pushed in the acceleration structure. This allows the
      // application to group shaders in the SBT in the same order as they are added in the AS, in
      // which case the value below represents the stride (4 bits representing the number of hit
      // groups) between two consecutive objects.
      0,

      // Parameter name: MissShaderIndex
      // Index of the miss shader to use in case several consecutive miss shaders are present in the
      // SBT. This allows to change the behavior of the program when no geometry have been hit, for
      // example one to return a sky color for regular rendering, and another returning a full
      // visibility value for shadow rays. This sample has only one miss shader, hence an index 0
      0,

      ray,
      payload
    );

    dstTexture[pixelCoord] = float4(payload.radiance, 1.f);
    // dstTexture[pixelCoord] = float4(distanceRGB, 1.f);
}

[shader("closesthit")] 
void ClosestHit(inout FPayload payload, Attributes attr) 
{
    const interlop::MeshVertex hitSurface = GetHitSurface(attr, renderResources, InstanceID());
    const interlop::FRaytracingMaterial material = GetGeometryMaterial(renderResources, InstanceID());

    float4 albedoEmissive = getAlbedoSample(hitSurface.texcoord, material.albedoTextureIndex, MeshSampler, material.albedoColor);
    // payload.radiance = float3(hitSurface.texcoord, material.albedoTextureIndex/1000.f);
    // Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.albedoTextureIndex];
    // payload.radiance = albedoTexture.SampleLevel(MeshSampler, hitSurface.texcoord, 0.f).xyz;
    payload.radiance = albedoEmissive.xyz;
    payload.distance = RayTCurrent();
}

[shader("miss")]
void Miss(inout FPayload payload : SV_RayPayload)
{
    payload.radiance = float3(0.2f, 0.2f, 0.8f);
    payload.distance = -1;
}
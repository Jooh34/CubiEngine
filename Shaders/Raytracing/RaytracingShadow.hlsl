// clang-format off

#include "ShaderInterlop/RenderResources.hlsli"
#include "Raytracing/Common.hlsl"
#include "Utils.hlsli"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space200);

ConstantBuffer<interlop::RaytracingShadowRenderResource> renderResources : register(b0);

SamplerState MeshSampler : register(s0);
SamplerState LinearSampler : register(s1);

[shader("raygeneration")] 
void RayGen()
{
    const uint2 pixelCoord = DispatchRaysIndex().xy;

    float2 ncdXY = (float2(pixelCoord) + 0.5f) / DispatchRaysDimensions().xy; // [0,1]
    ncdXY = ncdXY * 2.0f - 1.0f; // [-1,1]
    ncdXY.y = -ncdXY.y;

    const float2 uv = (float2(pixelCoord) + 0.5f) / DispatchRaysDimensions().xy;

    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::LightBuffer> lightBuffer = ResourceDescriptorHeap[renderResources.lightBufferIndex];
    
    const float depth = depthTexture.SampleLevel(MeshSampler, uv, 0.f);

    float4 p1 = mul(float4(ncdXY, depth, 1.0f), renderResources.invViewProjectionMatrix);
    float3 worldPosition = p1.xyz / p1.w;

    {
        // Directional Light
        uint DLIndex = 0;
        float lightIntensity = lightBuffer.intensityDistance[DLIndex][0];
        if (lightIntensity > 0.0f)
        {
            const float3 L = normalize(-lightBuffer.viewSpaceLightPosition[DLIndex].xyz);
            const float4 worldspaceL_ = mul(float4(L, 0.0f), sceneBuffer.inverseViewMatrix);
            const float3 worldspaceL = normalize(worldspaceL_.xyz);

            // shadow ray
            RayDesc ray;
            // ray.Origin = worldPosition.xyz + worldspaceL * 0.01f; // offset to avoid self intersection;
            ray.Origin = worldPosition.xyz;
            ray.Direction = worldspaceL;
            ray.TMin = 0.1f; // offset to avoid self intersection;
            ray.TMax = FP32Max;

            FShadowPayload shadowPayload;
            shadowPayload.visibility = 1.0f;

            uint traceRayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
            
            const uint hitGroupOffset = RayTypeShadow;
            const uint hitGroupGeoMultiplier = NumRayTypes;
            const uint missShaderIdx = RayTypeShadow;
            TraceRay(SceneBVH, traceRayFlags, 0xFFFFFFFF, hitGroupOffset, hitGroupGeoMultiplier, missShaderIdx, ray, shadowPayload);

            dstTexture[pixelCoord] = shadowPayload.visibility;
            return;
        }
    }
}

// below shader is not used. shadow.hlsl is used for RayTypeShadow.
[shader("closesthit")]
void ClosestHit(inout FShadowPayload payload, in Attributes attr)
{
    payload.visibility = 0.0f;
}

[shader("miss")]
void Miss(inout FShadowPayload payload)
{
    payload.visibility = 1.0f;
}
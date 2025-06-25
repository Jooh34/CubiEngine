// clang-format off
#pragma once

RayDesc generateViewRay(in float4x4 invViewProjectionMatrix, in float2 ncdXY)
{
    // Ray generation
    float4 rayStart = mul(float4(ncdXY, 1.0f, 1.0f), invViewProjectionMatrix);
    float4 rayEnd = mul(float4(ncdXY, 0.0f, 1.0f), invViewProjectionMatrix);

    rayStart.xyz /= rayStart.w;
    rayEnd.xyz /= rayEnd.w;
    float3 rayDir = normalize(rayEnd.xyz - rayStart.xyz);
    float rayLength = length(rayEnd.xyz - rayStart.xyz);
    
    // primary ray desc
    RayDesc rayDesc;
    rayDesc.Origin = rayStart.xyz;
    rayDesc.Direction = rayDir;
    rayDesc.TMin = EPS;
    rayDesc.TMax = rayLength;

    return rayDesc;
}

FPathTracePayload defaultPathTracePayload(const uint2 pixelCoord, uint sampleIndex)
{
    FPathTracePayload payload;
    payload.radiance = float3(0.0f, 0.0f, 0.0f);
    payload.distance = 0.0f;
    payload.depth = 0;
    payload.uv = pixelCoord;
    payload.sampleIndex = sampleIndex;

    return payload;
}
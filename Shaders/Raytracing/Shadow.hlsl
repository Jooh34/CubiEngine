#include "Raytracing/Common.hlsl"

[shader("closesthit")]
void ShadowClosestHit(inout FShadowPayload payload, in Attributes attr)
{
    payload.visibility = 0.0f;
}

[shader("miss")]
void ShadowMiss(inout FShadowPayload payload)
{
    payload.visibility = 1.0f;
}
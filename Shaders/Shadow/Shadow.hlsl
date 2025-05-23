#pragma once

#define SHADOW_FILTER_NONE 0
#define SHADOW_FILTER_PCF 1

uint getCascadeIndex(float depth, float nearZ, float farZ, uint numCascade, float4 distanceCSM)
{
    for (int i=0; i<numCascade-1; i++)
    {
        if (distanceCSM[i+1] > depth)
        {
            return i;
        }
    }

    return (numCascade-1);
}

float noFilter(float3 surfacePosition, float NoL, uint shadowDepthTextureIndex, float bias)
{
    Texture2D<float> shadowDepthTexture = ResourceDescriptorHeap[shadowDepthTextureIndex];

    float curDepth = shadowDepthTexture.Sample(linearWrapSampler, surfacePosition.xy).r;
    float shadow = (surfacePosition.z + bias < curDepth ? 1.0f : 0.0f); // ReversedZ
    return shadow;
}

float pcf(float3 surfacePosition, float NoL, uint shadowDepthTextureIndex, float bias)
{
    Texture2D<float> shadowDepthTexture = ResourceDescriptorHeap[shadowDepthTextureIndex];
    float width, height;
    shadowDepthTexture.GetDimensions(width, height);

    float2 texelSize = float2(1.0f, 1.0f) / float2(width, height);

    // Do PCF.
    float shadow = 0.0f;

    // Half kernel width.
    const float samplesOffset = 1.5;
    for (float x = -samplesOffset; x <= samplesOffset; x+=1.0)
    {
        for (float y = -samplesOffset; y <= samplesOffset; y+=1.0)
        {
            float curDepth = shadowDepthTexture.Sample(linearWrapSampler, surfacePosition.xy + float2(x, y) * texelSize).r;
            shadow += (surfacePosition.z + bias < curDepth ? 1.0f : 0.0f); // ReversedZ
        }
    }

    return shadow / 16.f;
}

float ChebyshevUpperBound(float2 Moments, float t)
{
    // One-tailed inequality valid if t > Moments.x
    float p = (t <= Moments.x);
    // Compute variance.
    float Variance = Moments.y - (Moments.x * Moments.x);
    Variance = max(Variance, 0.000001f);
    // Compute probabilistic upper bound.
    float d = t - Moments.x;
    float p_max = Variance / (Variance + d * d);
    return min(p, p_max);
}

float vsm(float3 surfacePosition, float NoL, uint vsmMomentTextureIndex, float bias)
{
    Texture2D<float2> vsmMomentTexture = ResourceDescriptorHeap[vsmMomentTextureIndex];
    float width, height;
    vsmMomentTexture.GetDimensions(width, height);

    float2 texelSize = float2(1.0f, 1.0f) / float2(width, height);

    // Do VSM.
    float2 moment = vsmMomentTexture.Sample(linearWrapSampler, surfacePosition.xy);
    float curDepth = surfacePosition.z - bias;

    float lit = ChebyshevUpperBound(moment, curDepth);

    return 1-lit;
}

float calculateShadow(float4 lightSpacPosition, float NoL, uint shadowDepthTextureIndex, uint vsmMomentTextureIndex, uint cascadeIndex, uint numCascadeShadowMap, float farZ, float shadowBias)
{
    float baseBias = (farZ * shadowBias) / 1000.f;
    float bias = max(baseBias * (1.0f - NoL), baseBias/10.f);
    
    // Do perspective divide
    float3 surfacePosition = lightSpacPosition.xyz / lightSpacPosition.w;

    // Convert x and y coord from [-1, 1] range to [0, 1].
    surfacePosition.x = surfacePosition.x * 0.5f + 0.5f;
    surfacePosition.y = surfacePosition.y * -0.5f + 0.5f;
    
    int gridX = cascadeIndex % 2;  // 0, 1 (가로 방향)
    int gridY = cascadeIndex / 2;  // 0, 1 (세로 방향)

    float2 topLeft = float2(gridX * 0.5f, gridY * 0.5f);

    if (numCascadeShadowMap > 1)
    {
        surfacePosition.x /= 2.f;
        surfacePosition.y /= 2.f;

        if (cascadeIndex == 1)
        {
            topLeft.x = 0.5f;
        }
        else if (cascadeIndex == 2)
        {
            topLeft.y = 0.5f;
        }
        else if (cascadeIndex == 3)
        {
            topLeft.x = 0.5f; 
            topLeft.y = 0.5f;
        }

        surfacePosition.x += topLeft.x;
        surfacePosition.y += topLeft.y;
    }

    float shadow = 0.f;
    if (vsmMomentTextureIndex == INVALID_INDEX)
    {
        shadow = pcf(surfacePosition, NoL, shadowDepthTextureIndex, bias);
    }
    else
    {
        shadow = vsm(surfacePosition, NoL, vsmMomentTextureIndex, bias);
    }
    return shadow;
}
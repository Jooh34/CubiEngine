#pragma once

#define SHADOW_FILTER_NONE 0
#define SHADOW_FILTER_PCF 1

uint getCascadeIndex(float viewSpacePositionZ, float nearZ, float farZ, uint numCascade)
{
    float distanceZ = farZ - nearZ;
    float ratioZ = (viewSpacePositionZ-nearZ) / distanceZ;
    float ratioCascade = 1.f / numCascade;

    for (int i=0; i<numCascade; i++)
    {
        float maxZ = ratioCascade * (i+1);
        if (ratioZ < maxZ)
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

float calculateShadow(float4 lightSpacPosition, float NoL, uint shadowDepthTextureIndex, uint cascadeIndex, uint numCascadeShadowMap, float farZ, float shadowBias)
{
    float baseBias = (farZ * shadowBias) / 1000.f;
    float bias = max(baseBias * (1.0f - NoL), baseBias/10.f);
    
    // Do perspective divide
    float3 surfacePosition = lightSpacPosition.xyz / lightSpacPosition.w;

    // Convert x and y coord from [-1, 1] range to [0, 1].
    surfacePosition.x = surfacePosition.x * 0.5f + 0.5f;
    surfacePosition.y = surfacePosition.y * -0.5f + 0.5f;
    float2 topLeft = float2(0.f, 0.f);
    float width, height;
    Texture2D<float> shadowDepthTexture = ResourceDescriptorHeap[shadowDepthTextureIndex];
    shadowDepthTexture.GetDimensions(width, height);

    if (numCascadeShadowMap > 1)
    {
        surfacePosition.x /= 2.f;
        surfacePosition.y /= 2.f;

        if (cascadeIndex == 1)
        {
            topLeft.x = width / 2.f;
        }
        else if (cascadeIndex == 2)
        {
            topLeft.y = height / 2.f;
        }
        else if (cascadeIndex == 3)
        {
            topLeft.x = width / 2.f; 
            topLeft.y = height / 2.f;
        }

        surfacePosition.x += topLeft.x;
        surfacePosition.y += topLeft.y;
    }

    // float shadow = noFilter(surfacePosition, NoL, shadowDepthTextureIndex, bias);
    float shadow = pcf(surfacePosition, NoL, shadowDepthTextureIndex, bias);
    return shadow;
}
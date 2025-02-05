#pragma once

float calculateShadow(float4 lightSpacPosition, float NoL, uint shadowDepthTextureIndex)
{
    // Do perspective divide
    float3 shadowPosition = lightSpacPosition.xyz / lightSpacPosition.w;

    if (shadowPosition.z > 1.0f)
    {
        return 0.0f;
    }

    // Convert x and y coord from [-1, 1] range to [0, 1].
    shadowPosition.x = shadowPosition.x * 0.5f + 0.5f;
    shadowPosition.y = shadowPosition.y * -0.5f + 0.5f;

    Texture2D<float> shadowDepthTexture = ResourceDescriptorHeap[shadowDepthTextureIndex];

    // Get texture dimensions for texel size.
    float width, height;
    shadowDepthTexture.GetDimensions(width, height);

    float2 texelSize = float2(1.0f, 1.0f) / float2(width, height);
    const float bias = max(0.05f * (1.0f - NoL), 0.005f);

    // Do PCF.
    float shadow = 0.0f;

    // Half kernel width.
    const float samplesOffset = 1.5;
    for (float x = -samplesOffset; x <= samplesOffset; x+=1.0)
    {
        for (float y = -samplesOffset; y <= samplesOffset; y+=1.0)
        {
            float curDepth = shadowDepthTexture.Sample(linearWrapSampler, shadowPosition.xy + float2(x, y) * texelSize).r;
            shadow += (shadowPosition.z + bias < curDepth ? 1.0f : 0.0f); // ReversedZ
        }
    }

    return shadow / 16.f;
}
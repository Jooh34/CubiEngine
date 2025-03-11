// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::GenerateHistogramRenderResource> renderResources : register(b0);

#define GROUP_SIZE 256
#define THREADS_X 16
#define THREADS_Y 16

#define RGB_TO_LUM float3(0.2125, 0.7154, 0.0721)
#define EPSILON 0.005

// **Shared Memory for Histogram Calculation**
groupshared uint histogramShared[GROUP_SIZE];

uint colorToBin(float3 hdrColor, float minLogLum, float inverseLogLumRange) {
    // Convert our RGB value to Luminance, see note for RGB_TO_LUM macro above
    float lum = dot(hdrColor, RGB_TO_LUM);

    // Avoid taking the log of zero
    if (lum < EPSILON) {
        return 0;
    }

    // Calculate the log_2 luminance and express it as a value in [0.0, 1.0]
    // where 0.0 represents the minimum luminance, and 1.0 represents the max.
    float logLum = clamp((log2(lum) - minLogLum) * inverseLogLumRange, 0.0, 1.0);

    // Map [0, 1] to [1, 255]. The zeroth bin is handled by the epsilon check above.
    return uint(logLum * 254.0 + 1.0);
}

[RootSignature(BindlessRootSignature)][numthreads(THREADS_X, THREADS_Y, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
    Texture2D<float4> sceneTexture = ResourceDescriptorHeap[renderResources.sceneTextureIndex];
    RWStructuredBuffer<uint> histogramBuffer = ResourceDescriptorHeap[renderResources.histogramBufferIndex];

    uint2 dim;
    sceneTexture.GetDimensions(dim.x, dim.y);

    // Initialize shared histogram
    histogramShared[groupIndex] = 0;
    GroupMemoryBarrierWithGroupSync();

    if (dispatchThreadID.x < dim.x && dispatchThreadID.y < dim.y) {
        float3 hdrColor = sceneTexture[dispatchThreadID.xy].xyz;
        uint binIndex = colorToBin(hdrColor, renderResources.minLogLuminance, renderResources.inverseLogLuminanceRange);
        InterlockedAdd(histogramShared[binIndex], 1);
    }

    // Synchronize workgroup before writing to global buffer
    GroupMemoryBarrierWithGroupSync();

    // Store computed histogram in global buffer using atomic operations
    InterlockedAdd(histogramBuffer[groupIndex], histogramShared[groupIndex]);
}
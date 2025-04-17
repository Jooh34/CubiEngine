// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::CalculateAverageLuminanceRenderResource> renderResources : register(b0);

#define GROUP_SIZE 256
#define THREADS_X 16
#define THREADS_Y 16

groupshared uint histogramShared[GROUP_SIZE];

[numthreads(THREADS_X, THREADS_Y, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
    RWStructuredBuffer<uint> histogramBuffer = ResourceDescriptorHeap[renderResources.histogramBufferIndex];
    RWStructuredBuffer<float4> averageLuminanceBuffer = ResourceDescriptorHeap[renderResources.averageLuminanceBufferIndex];
    float luminanceLastFrame = averageLuminanceBuffer[0].x;

    float minLogLum = renderResources.minLogLuminance;
    float logLumRange = renderResources.logLumRange;  
    float timeCoeff = renderResources.timeCoeff;
    float numPixels = renderResources.numPixels;
    float timeDelta = 0.033f;
    
    // Get the count from the histogram buffer
    uint countForThisBin = histogramBuffer[groupIndex];
    histogramShared[groupIndex] = countForThisBin * groupIndex;

    GroupMemoryBarrierWithGroupSync();

    // Reset the count stored in the buffer in anticipation of the next pass
    histogramBuffer[groupIndex] = 0;

    // **Reduction Loop - Weighted Luminance Calculation**
    for (uint cutoff = (GROUP_SIZE >> 1); cutoff > 0; cutoff >>= 1) {
        if (groupIndex < cutoff) {
            histogramShared[groupIndex] += histogramShared[groupIndex + cutoff];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // **Final Calculation (Executed by Thread Index 0)**
    if (groupIndex == 0) {
        // Compute the weighted log average
        float weightedLogAverage = (histogramShared[0] / max(numPixels - float(countForThisBin), 1.0)) - 1.0;

        // Convert from histogram space to actual luminance
        float weightedAvgLum = exp2(((weightedLogAverage / 254.0) * logLumRange) + minLogLum);

        // **Read last frame's luminance for smooth exposure adaptation**
        float adaptedLum = luminanceLastFrame + (weightedAvgLum - luminanceLastFrame) * (1 - exp(-timeDelta * timeCoeff));

        // Store new adapted luminance value
        averageLuminanceBuffer[0] = float4(adaptedLum, 0, 0, 0);
    }
}
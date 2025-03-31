// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::SSGIResolveRenderResource> renderResources : register(b0);

float3 GetHistoryScreenPosition(float2 ScreenPosition, float DeviceZ, float4x4 ClipToPrevClip)
{
    float3 HistoryScreenPosition = float3(ScreenPosition, DeviceZ);
    float4 ThisClip = float4(HistoryScreenPosition, 1);
    float4 PrevClip = mul(ThisClip, ClipToPrevClip); 
    float3 PrevScreen = PrevClip.xyz / PrevClip.w;

    // float3 Velocity = HistoryScreenPosition - PrevScreen;
    // HistoryScreenPosition -= Velocity;
    return PrevScreen;
}

float4 GetHistoryScreenPositionTest(float2 ScreenPosition, float DeviceZ, float4x4 ClipToPrevClip)
{
    float3 HistoryScreenPosition = float3(ScreenPosition, DeviceZ);
    float4 ThisClip = float4(HistoryScreenPosition, 1);
    float4 PrevClip = mul(ThisClip, ClipToPrevClip); 
    return PrevClip;
}

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> denoisedTexture = ResourceDescriptorHeap[renderResources.denoisedTextureIndex];
    Texture2D<float4> historyTexture = ResourceDescriptorHeap[renderResources.historyTextureIndex];
    Texture2D<float2> velocityTexture = ResourceDescriptorHeap[renderResources.velocityTextureIndex];
    Texture2D<float> prevDepthTexture = ResourceDescriptorHeap[renderResources.prevDepthTextureIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];

    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    RWTexture2D<float> numFramesAccumulatedTexture = ResourceDescriptorHeap[renderResources.numFramesAccumulatedTextureIndex];
    
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;
    // float2 velocity = velocityTexture.Sample(linearWrapSampler, uv);
    // const float2 prevUV = uv - velocity;


    // depth threshold
    float disocclusionDistanceThreshold = 0.005f;

    float3 prevUVZ = GetHistoryScreenPosition(UVToClip(uv), depthTexture[dispatchThreadID.xy], sceneBuffer.clipToPrevClip);
    prevUVZ.xy = ClipToUV(prevUVZ.xy);
    float2 prevUV = prevUVZ.xy;

    float prevDepth = ConvertFromDeviceZ(prevDepthTexture.Sample(pointClampSampler, prevUV), sceneBuffer.invDeviceZToWorldZTransform);
    float historySceneDepth = ConvertFromDeviceZ(prevUVZ.z, sceneBuffer.invDeviceZToWorldZTransform);

    const float distanceToHistoryValue = abs(historySceneDepth - prevDepth);
	bool bDiscard  = distanceToHistoryValue >= historySceneDepth * disocclusionDistanceThreshold;

    bool bHistoryWasOnscreen = all(prevUV < float2(1.f, 1.f)) && all(prevUV > float2(0.f, 0.f)) && !bDiscard;

    float3 radiance = denoisedTexture.Sample(pointClampSampler, uv).xyz;
    float3 historyColor = historyTexture.Sample(pointClampSampler, prevUV).xyz;
    if (isnan(radiance.x) || isnan(radiance.y) || isnan(radiance.z))
    {
        radiance = float3(0.f, 0.f, 0.f);
    }
    if (isnan(historyColor.x) || isnan(historyColor.y) || isnan(historyColor.z))
    {
        historyColor = float3(0.f, 0.f, 0.f);
    }

    int maxHistoryFrame = renderResources.maxHistoryFrame;
    float numFramesAccumulated = numFramesAccumulatedTexture[dispatchThreadID.xy] * maxHistoryFrame + 1.f;
    numFramesAccumulated = bHistoryWasOnscreen ? numFramesAccumulated : 0.f;

    float alpha = 1.0f / (1.0f + numFramesAccumulated);
    float3 color = lerp(historyColor, radiance, alpha);

    numFramesAccumulatedTexture[dispatchThreadID.xy] = saturate(numFramesAccumulated / maxHistoryFrame);

    dstTexture[dispatchThreadID.xy] = float4(color, 1.f);
}
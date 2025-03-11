// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
ConstantBuffer<interlop::RaycastDiffuseRenderResource> renderResources : register(b0);

static const int stepCount = 32;

struct FTraceResult
{
    bool hit;
    float2 hitUV;
    float mask;
};
/*
// From UnrealnEngine
// Returns distance along ray that the first hit occurred, or negative on miss
float CastScreenSpaceShadowRay(
	float3 RayOriginTranslatedWorld, float3 RayDirection, float RayLength, int NumSteps,
	float Dither, float CompareToleranceScale, bool bHairNoShadowLight,
	out float2 HitUV)
{
	float4 RayStartClip = mul(float4(RayOriginTranslatedWorld, 1), View.TranslatedWorldToClip);
	float4 RayDirClip = mul(float4(RayDirection * RayLength, 0), View.TranslatedWorldToClip);
	float4 RayEndClip = RayStartClip + RayDirClip;

	float3 RayStartScreen = RayStartClip.xyz / RayStartClip.w;
	float3 RayEndScreen = RayEndClip.xyz / RayEndClip.w;

	float3 RayStepScreen = RayEndScreen - RayStartScreen;

	float3 RayStartUVz = float3(RayStartScreen.xy * View.ScreenPositionScaleBias.xy + View.ScreenPositionScaleBias.wz, RayStartScreen.z);
	float3 RayStepUVz = float3(RayStepScreen.xy * View.ScreenPositionScaleBias.xy, RayStepScreen.z);

	float4 RayDepthClip = RayStartClip + mul(float4(0, 0, RayLength, 0), View.ViewToClip);
	float3 RayDepthScreen = RayDepthClip.xyz / RayDepthClip.w;

	const float StepOffset = Dither - 0.5f;
	const float Step = 1.0 / NumSteps;

	const float CompareTolerance = abs(RayDepthScreen.z - RayStartScreen.z) * Step * CompareToleranceScale;

	float SampleTime = StepOffset * Step + Step;

	float FirstHitTime = -1.0;

	const float StartDepth = LookupDeviceZ(RayStartUVz.xy);

	UNROLL
	for (int i = 0; i < NumSteps; i++)
	{
		float3 SampleUVz = RayStartUVz + RayStepUVz * SampleTime;
		float SampleDepth = LookupDeviceZ(SampleUVz.xy);

		// Avoid self-intersection with the start pixel (exact comparison due to point sampling depth buffer)
		// Exception is made for hair for occluding transmitted light with non-shadow casting light
		if (SampleDepth != StartDepth || bHairNoShadowLight)
		{
			float DepthDiff = SampleUVz.z - SampleDepth;
			bool Hit = abs(DepthDiff + CompareTolerance) < CompareTolerance;

			if (Hit)
			{
				HitUV = SampleUVz.xy;

				// Off screen masking
				bool bValidUV = all(and (0.0 < SampleUVz.xy, SampleUVz.xy < 1.0));

				return bValidUV ? (RayLength * SampleTime) : -1.0;
			}
		}

		SampleTime += Step;
	}

	return -1;
}
*/

float2 ClipToUV(float2 ClipXY)
{
    float2 uv = ClipXY*0.5f+0.5f;
    uv.y = 1.f - uv.y;
    return uv;
}

FTraceResult TraceScreenSpaceRay(
    Texture2D<float> depthTexture,
    float3 rayOrigin,
    float3 rayDirection,
    float rayLength,
    float4x4 projectionMatrix,
    float CompareToleranceScale,
    int NumSteps
)
{
    FTraceResult result = (FTraceResult)0;
    result.hit = false;
    result.hitUV = float2(0,0);
    result.mask = 0.f;

    float4 RayStartClip = mul(float4(rayOrigin, 1), projectionMatrix);
	float4 RayDirClip = mul(float4(rayDirection * rayLength, 0), projectionMatrix);
	float4 RayEndClip = RayStartClip + RayDirClip;

	float3 RayStartScreen = RayStartClip.xyz / RayStartClip.w;
	float3 RayEndScreen = RayEndClip.xyz / RayEndClip.w;
    
	float3 RayStepScreen = RayEndScreen - RayStartScreen;

    float3 RayStartUVz = float3(RayStartScreen.xy, RayStartScreen.z); // Todo bias
	float3 RayStepUVz = float3(RayStepScreen.xy, RayStepScreen.z); // Todo bias
    
	float4 RayDepthClip = RayStartClip + mul(float4(0, 0, rayLength, 0), projectionMatrix);
	float3 RayDepthScreen = RayDepthClip.xyz / RayDepthClip.w;

	// const float StepOffset = Dither - 0.5f;
	const float Step = 1.0 / NumSteps;

	const float CompareTolerance = abs(RayDepthScreen.z - RayStartScreen.z) * Step * CompareToleranceScale;

	float SampleTime = Step*3; // start bias

	float FirstHitTime = -1.0;

	const float StartDepth = depthTexture.Sample(pointClampSampler, ClipToUV(RayStartUVz.xy));

    // temp
    for (int i = 0; i < NumSteps; i++)
	{
		float3 SampleUVz = RayStartUVz + RayStepUVz * SampleTime;
		float SampleDepth = depthTexture.Sample(pointClampSampler, ClipToUV(SampleUVz.xy));

        // if (i==0)
        // {
        //     SampleUVz = RayStartUVz + RayStepUVz * 30 * SampleTime;
        //     result.hit = true;
        //     result.hitUV = SampleUVz.xy*0.5f+0.5f;
        //     result.hitUV.y = 1.f-result.hitUV.y;
        //     return result;
        // }

		if (SampleDepth != StartDepth)
		{
			float DepthDiff = SampleUVz.z - SampleDepth;
			bool Hit = abs(DepthDiff) < CompareTolerance;

			if (Hit)
			{
				result.hitUV = ClipToUV(SampleUVz.xy);

				// Off screen masking
				result.hit = all(and(-1.0 < SampleUVz.xy, SampleUVz.xy < 1.0));
                if (result.hit)
                {
                    result.mask = 0.1f * rayLength / (rayLength * SampleTime);
                }
                return result;
			}
		}

		SampleTime += Step;
	}

    return result;
}

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> sceneColorTexture = ResourceDescriptorHeap[renderResources.sceneColorTextureIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];
    Texture2D<float4> stochasticNormalTexture = ResourceDescriptorHeap[renderResources.stochasticNormalTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    float2 wh = float2(renderResources.width, renderResources.height);
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport; 
    const float2 pixel = (dispatchThreadID.xy + 0.5f);

    const float3 normal = stochasticNormalTexture.Sample(pointClampSampler, uv).xyz;
    const float depth = depthTexture.Sample(pointClampSampler, uv);

    float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);
    
    // FTraceResult TraceScreenSpaceRay(
    //     Texture2D<float> depthTexture,
    //     float3 rayOrigin,
    //     float3 rayDirection,
    //     float length,
    //     float4x4 projectionMatrix,
    //     float CompareToleranceScale,
    //     int NumSteps
    // )

    float RayLength = renderResources.rayLength;
    int NumSteps = renderResources.numSteps;
    float ssgiIntensity = renderResources.ssgiIntensity;

    FTraceResult result = TraceScreenSpaceRay(depthTexture, viewSpacePosition, normal, RayLength, sceneBuffer.projectionMatrix, renderResources.compareToleranceScale, NumSteps);
    if (!result.hit)
    {
		dstTexture[dispatchThreadID.xy] = float4(0, 0, 0, 1);
    }
	else
	{
		float3 hitColor = sceneColorTexture.Sample(pointClampSampler, result.hitUV).xyz;

		// why ?
		if (isnan(hitColor.x) || isnan(hitColor.y) || isnan(hitColor.z)) {
			hitColor = float3(0, 0, 0);
		}
		dstTexture[dispatchThreadID.xy] = float4(hitColor * ssgiIntensity, 1.f);
	}
}
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

		if (SampleDepth != StartDepth)
		{
			float DepthDiff = SampleUVz.z - SampleDepth;
			bool Hit = abs(DepthDiff+CompareTolerance) < CompareTolerance;

			if (Hit)
			{
				result.hitUV = ClipToUV(SampleUVz.xy);

				// Off screen masking
				result.hit = all(and(-1.0 < SampleUVz.xy, SampleUVz.xy < 1.0));
                if (result.hit)
                {
                    result.mask = rayLength / (rayLength * SampleTime);
                }
                return result;
			}
		}

		SampleTime += Step;
	}

    return result;
}

void GenerateStochasticNormal(const float3 normal, float2 u, out float3 stochasticNormal, out float pdf)
{
    if (renderResources.stochasticNormalSamplingMethod == 0)
    {
        float3 t = float3(0.0f, 0.0f, 0.0f);
        float3 s = float3(0.0f, 0.0f, 0.0f);
        stochasticNormal = tangentToWorldCoords(UniformSampleHemisphere(u), normal.xyz, s, t);
		pdf = 1.f / (2*PI);
    }
    else if (renderResources.stochasticNormalSamplingMethod == 1)
	{
        float NdotL;
        ImportanceSampleCosDir(u, normal.xyz, stochasticNormal, NdotL, pdf);
    }
	else if (renderResources.stochasticNormalSamplingMethod == 2)
	{
		ConcentricSampleDisk(u, normal, stochasticNormal, pdf);
	}
	else
	{
		float3x3 TangentBasis = GetTangentBasis(normal);
		float4 result = CosineSampleHemisphereConcentric(u);
		float3 TangentL = result.xyz;
		pdf = result.w;
		stochasticNormal = mul(TangentL, TangentBasis);
	}
}

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> sceneColorTexture = ResourceDescriptorHeap[renderResources.sceneColorTextureIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];
    Texture2D<float4> GBufferBTexture = ResourceDescriptorHeap[renderResources.GBufferBTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    float2 wh = float2(renderResources.width, renderResources.height);
    float2 invViewport = float2(1.f/(float)renderResources.width, 1.f/(float)renderResources.height);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport; 
    const float2 pixel = (dispatchThreadID.xy + 0.5f);

	float3 normal = GBufferBTexture.Sample(pointClampSampler, uv).xyz;

    const float depth = depthTexture.Sample(pointClampSampler, uv);
    float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);

    float RayLength = renderResources.rayLength;
    int NumSteps = renderResources.numSteps;
    float ssgiIntensity = renderResources.ssgiIntensity;

	float3 radiance = float3(0,0,0);
	int numSample = renderResources.numSamples;
	if (numSample <= 0) numSample = 1;

	for (int i=0; i<numSample; i++)
	{
		float3 stochasticNormal;
		float pdf;
		float noise = InterleavedGradientNoiseByIntel(dispatchThreadID.xy, renderResources.frameCount*numSample+i);
		float2 u = float2(noise, noise);

		GenerateStochasticNormal(normal, u, stochasticNormal, pdf);
		// {
		// 	dstTexture[dispatchThreadID.xy] = float4(stochasticNormal, 1.f);
		// 	return;
		// }

		FTraceResult result = TraceScreenSpaceRay(depthTexture, viewSpacePosition, stochasticNormal, RayLength, sceneBuffer.projectionMatrix, renderResources.compareToleranceScale, NumSteps);
		if (result.hit)
		{
			float3 hitColor = sceneColorTexture.Sample(pointClampSampler, result.hitUV).xyz;

			radiance += float3(hitColor * ssgiIntensity * result.mask * pdf);
		}
	}
	radiance = radiance / numSample;

	dstTexture[dispatchThreadID.xy] = float4(radiance, 1.f);
}
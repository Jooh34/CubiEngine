// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
ConstantBuffer<interlop::SSAORenderResource> renderResources : register(b0);

float2 ClipToUV(float2 ClipXY)
{
    float2 uv = ClipXY*0.5f+0.5f;
    uv.y = 1.f - uv.y;
    return uv;
}

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> GBufferB = ResourceDescriptorHeap[renderResources.GBufferBIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[renderResources.depthTextureIndex];
    RWTexture2D<float> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    ConstantBuffer<interlop::SSAOKernelBuffer> kernelBuffer = ResourceDescriptorHeap[renderResources.SSAOKernelBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    uint frameCount = renderResources.frameCount;
    uint kernelSize = renderResources.kernelSize;
    float kernelRadius = renderResources.kernelRadius;
    float depthBias = renderResources.depthBias;

    float dstWidth, dstHeight;
    dstTexture.GetDimensions(dstWidth, dstHeight);
    if (dispatchThreadID.x >= dstWidth || dispatchThreadID.y >= dstHeight)
    {
        return;
    }

    float2 invViewport = float2(1.f/dstWidth, 1.f/dstHeight);
    const float2 uv = (dispatchThreadID.xy + 0.5f) * invViewport;

    const float depth = depthTexture.Sample(pointClampSampler, uv);
    const float3 normal = GBufferB.Sample(pointClampSampler, uv).xyz;
    const float3 viewSpacePosition = viewSpaceCoordsFromDepthBuffer(depth, uv, sceneBuffer.inverseProjectionMatrix);
    const float4 worldSpacePosition = mul(float4(viewSpacePosition, 1.0f), sceneBuffer.inverseViewMatrix);

    // float3 noise = normalize(float3(
    //     InterleavedGradientNoise(uv, frameCount*3),
    //     InterleavedGradientNoise(uv, frameCount*3+1),
    //     InterleavedGradientNoise(uv, frameCount*3+2)
    // ));

    float3 tangent = generateTangent(normal).xyz;
    // float3 tangent   = normalize(noise - normal * dot(noise, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN       = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0f;
    for (int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        float3 sampleVector = normalize(mul(kernelBuffer.kernel[i].xyz, TBN));
        float3 samplePos = viewSpacePosition + sampleVector * kernelRadius;

        const float4 clipspacePosition = mul(float4(samplePos, 1.0f), sceneBuffer.projectionMatrix);
        float3 clipXYZ = clipspacePosition.xyz / clipspacePosition.w;
        float2 peekUV = ClipToUV(clipXYZ.xy);

        float sampleDepth = depthTexture.Sample(pointClampSampler, peekUV);
        float3 sampleDepthViewSpace = viewSpaceCoordsFromDepthBuffer(sampleDepth, peekUV, sceneBuffer.inverseProjectionMatrix);

        float rangeCheck = 1.f;
        if (renderResources.bUseRangeCheck)
        {
            rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(samplePos.z - sampleDepthViewSpace.z));
        }
        occlusion += (sampleDepth >= clipXYZ.z + depthBias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion /= kernelSize;
    dstTexture[dispatchThreadID.xy] = 1.f-occlusion;
}
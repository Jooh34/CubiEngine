// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::GaussianBlurRenderResource> renderResources : register(b0);

static const float weight[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);
    bool horizontal = bool(renderResources.bHorizontal);

    float3 result = srcTexture.Sample(pointClampSampler, uvCoords).rgb * weight[0];
    for(int i = 0; i < 5; ++i)
    {
        float2 offset = horizontal ? float2(texelSize.x * i, 0) : float2(0, texelSize.y * i);
        float2 uv = uvCoords + offset;
        
        result += srcTexture.Sample(pointClampSampler, uv).rgb * weight[i];
        result += srcTexture.Sample(pointClampSampler, uv).rgb * weight[i];
    }

    dstTexture[coord] = float4(result, 1.f);
}
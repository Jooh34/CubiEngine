// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::GaussianBlurWRenderResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];
    
    float dstWidth, dstHeight;
    dstTexture.GetDimensions(dstWidth, dstHeight);
    if (coord.x >= dstWidth || coord.y >= dstHeight)
    {
        return;
    }

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);
    bool horizontal = bool(renderResources.bHorizontal);
    uint kernelSize = renderResources.kernelSize;

    float3 result = float3(0,0,0);
    int Q = kernelSize/4;
    float shift = -floor(kernelSize/2); // Shift to center of kernel
    if (kernelSize % 2 == 0)
    {
        shift += 0.5f;
    }

    for (int i = 0; i < Q; ++i)
    {
        float4 weights = renderResources.weights[i];
        for (int j=0; j<4; j++)
        {
            int index = i*4+j+shift;
            float weight = weights[j];
            float2 offset = horizontal ? float2(texelSize.x * index, 0) : float2(0, texelSize.y * index);
            float2 uv = uvCoords + offset;
            float4 color = srcTexture.Sample(pointClampSampler, uv);
            result += color.xyz * weight;
        }
    }

    int Rem = kernelSize%4;
    for (int j=0; j<Rem; j++)
    {
        int index = Q*4+j+shift;
        float4 weights = renderResources.weights[Q];
        float weight = weights[j];

        float2 offset = horizontal ? float2(texelSize.x * index, 0) : float2(0, texelSize.y * index);
        float2 uv = uvCoords + offset;
        float4 color = srcTexture.Sample(pointClampSampler, uv);
        result += color.xyz * weight;
    }

    if (renderResources.additiveTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> additiveTexture = ResourceDescriptorHeap[renderResources.additiveTextureIndex];
        float3 additive_color = additiveTexture.Sample(linearClampSampler, uvCoords).rgb;
        result += additive_color;
    }
    dstTexture[coord] = float4(result, 1.f);
}
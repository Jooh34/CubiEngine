// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

float3 GetIrradiance(float3 N, in TextureCube<float4> EnvMap)
{
    float3 PrefilteredColor = float3(0,0,0);
    const uint NumSamples = 1024;
    const float InvNumSamples = 1 / (float)NumSamples;
    float resolution = 1024.f;

    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    computeBasisVectors(N, s, t);

    for( uint i = 0; i < NumSamples; i++ )
    {
        float2 Xi = Hammersley( i, InvNumSamples );
        const float3 L = tangentToWorldCoords(UniformSampleHemisphere(Xi), N, s, t);
        float NoL = saturate(dot(N, L));
        if (NoL > 0.0)
        {            
            PrefilteredColor += EnvMap.SampleLevel(linearClampSampler, L, 0).rgb * NoL;
        }
    }
    return PrefilteredColor * 2.f * InvNumSamples;
}

ConstantBuffer<interlop::GeneratePrefilteredCubemapResource> renderResources : register(b0);

[RootSignature(BindlessRootSignature)][numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;
    uint faceIndex = dispatchThreadID.z;
    const float2 uv = dispatchThreadID.xy * renderResources.texelSize;

    TextureCube<float4> srcMipTexture = ResourceDescriptorHeap[renderResources.srcMipSrvIndex];
    RWTexture2DArray<float4> dstMipTexture = ResourceDescriptorHeap[renderResources.dstMipUavIndex];

    const float3 N = normalize(getSamplingVector(uv, dispatchThreadID));
    float3 color = GetIrradiance(N, srcMipTexture);

    dstMipTexture[dispatchThreadID] = float4(color, 1.f);
}
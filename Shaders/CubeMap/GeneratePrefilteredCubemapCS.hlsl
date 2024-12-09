// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

float3 PrefilterEnvMap(float Roughness, float3 V, in TextureCube<float4> EnvMap)
{
    float3 t = float3(0.0f, 0.0f, 0.0f);
    float3 s = float3(0.0f, 0.0f, 0.0f);

    float3 N = V;

    computeBasisVectors(N,s,t);
    float3 PrefilteredColor = float3(0,0,0);
    const uint NumSamples = 1024;
    const float InvNumSamples = 1 / (float)NumSamples;

    float TotalWeight = 0;
    for( uint i = 0; i < NumSamples; i++ )
    {
        float2 Xi = Hammersley( i, InvNumSamples );
        float3 H = ImportanceSampleGGX( Xi, Roughness, N, s, t);
        float3 L = reflect(-V, H);
        float NoL = saturate( dot( N, L ) );
        PrefilteredColor += EnvMap.SampleLevel( linearClampSampler , L, 0 ).rgb * NoL;
        TotalWeight += NoL;
    }

    return PrefilteredColor / TotalWeight;
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

    const float roughness = (float)renderResources.mipLevel / (float)(renderResources.totalMipLevel);
    const float3 V = normalize(getSamplingVector(uv, dispatchThreadID));

    float3 color = PrefilterEnvMap(roughness, V, srcMipTexture);

    dstMipTexture[dispatchThreadID] = float4(color, 1.f);
}
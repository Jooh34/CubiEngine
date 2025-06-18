// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/Sampling.hlsli"

float3 PrefilterEnvMap(float Roughness, float3 V, in TextureCube<float4> EnvMap)
{
    float3 N = V;

    float3 PrefilteredColor = float3(0,0,0);
    const uint NumSamples = 1024;
    const float InvNumSamples = 1 / (float)NumSamples;

    float TotalWeight = 0;
    for( uint i = 0; i < NumSamples; i++ )
    {
        float2 Xi = Hammersley( i, InvNumSamples );
        float3 H = ImportanceSampleGGX( Xi, Roughness, N);
        float3 L = reflect(-V, H);
        float NoL = saturate( dot( N, L ) );
        if (NoL > 0)
        {
            float resolution = 1024.f;
            float NoH = saturate(dot(N, H));
            float D = D_GGX(pow4(Roughness), NoH);
            // float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
            // but since V = N => VoH == NoH
            float pdf = D / 4.0; 

            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(NumSamples) * pdf + EPS);

            float mipLevel = Roughness == 0.0 ? 0.0 : max(0.5 * log2(saSample / saTexel), 0); 

            PrefilteredColor += EnvMap.SampleLevel( linearClampSampler , L, mipLevel).rgb * NoL;
            TotalWeight += NoL;
        }
    }

    return PrefilteredColor / max(TotalWeight, EPS);
}

ConstantBuffer<interlop::GeneratePrefilteredCubemapResource> renderResources : register(b0);

[numthreads(8, 8, 1)]
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
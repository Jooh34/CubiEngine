// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

ConstantBuffer<interlop::GenerateBRDFLutRenderResource> renderResources : register(b0);

float schlickBeckmannGS(float cosTheta, const float roughnessFactor)
{
    float k = (roughnessFactor * roughnessFactor) / 2.0f;

    return cosTheta / (max((cosTheta * (1.0f - k) + k), MIN_FLOAT_VALUE));
}

float smithGeometryFunction(float cosLI, float cosLO, const float roughnessFactor)
{
    return schlickBeckmannGS(cosLI, roughnessFactor) *
           schlickBeckmannGS(cosLO, roughnessFactor);
}

[numthreads(32, 32, 1)] 
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID) 
{
    RWTexture2D<float4> lutTexture = ResourceDescriptorHeap[renderResources.textureUavIndex];

    float textureWidth, textureHeight;
    lutTexture.GetDimensions(textureWidth, textureHeight);

    float nDotV = (dispatchThreadID.x + 1.0f) / textureWidth;
    float roughness = (dispatchThreadID.y + 1.0f) / textureHeight;

    nDotV  = saturate(nDotV);

    float3 V = float3(sqrt(1.0f - nDotV * nDotV), 0.0f, nDotV);

    // dfg1 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - (1 - vDotH)^5))dwi]
    // dfg2 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - vDotH)^5 dwi]
    float dfg1 = 0.0f;
    float dfg2 = 0.0f;
    float diffuse = 0.0f;

    uint numSample = 1024;
    float invNumSample = 1.f/ numSample; 
    float3 N = float3(0,0,1);

    for (uint i = 0u; i < numSample; ++i)
    {
        float2 Xi = Hammersley(i, invNumSample);
        const float3 H = ImportanceSampleGGX(Xi, roughness, N);

        float3 L = reflect(-V, H);

        // Using the fact that N = (0, 0, 1).
        float nDotL = max(L.z, 0.0f);
        float nDotH = max(H.z, 0.0f);
        float vDotH = saturate(dot(V, H));

        // specular term
        if (nDotL > 0.0)
        {
            // The microfacet BRDF formulation.
            const float g = smithGeometryFunction(nDotL, nDotV, roughness);
            const float gv = g * vDotH / (nDotH * nDotV);
            const float f = pow(1.0 - vDotH, 5);

            dfg1 += (1 - f) * gv;
            dfg2 += f * gv;

        }
            
        // diffuse term
        Xi = frac(Xi + 0.5f);
        float pdf;
        ImportanceSampleCosDir(Xi, N, L, nDotL, pdf);
        if (nDotL > 0.0)
        {
            float LdotH = saturate(dot(L, normalize(V+L)));
            float NdotV = saturate(dot(N, V));
            diffuse += Fr_DisneyDiffuse(NdotV, nDotL, LdotH, sqrt(roughness));
        }
    }

    lutTexture[dispatchThreadID.xy] = float4(dfg1, dfg2, diffuse, 1.f) * invNumSample;
}
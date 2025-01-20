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

    float NoV = (dispatchThreadID.x + 1.0f) / textureWidth;
    float roughness = (dispatchThreadID.y + 1.0f) / textureHeight;

    NoV  = saturate(NoV);

    float3 V = float3(sqrt(1.0f - NoV * NoV), 0.0f, NoV);

    // dfg1 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - (1 - VoH)^5))dwi]
    // dfg2 is the integral term involving F0 * integral[brdf . (n.wi) . (1 - VoH)^5 dwi]
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

        float3 L = 2.0 * dot(V, H) * H - V;

        // Using the fact that N = (0, 0, 1).
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float VoH = saturate(dot(V, H));

        // specular term
        if (NoL > 0.0)
        {
            // The microfacet BRDF formulation.
            // const float g = smithGeometryFunction(NoL, NoV, roughness);
            const float g = V_SmithGGXCorrelated(NoV, NoL, roughness);
            const float gv = g * VoH * NoL / (NoH);
            const float f = pow(1.0 - VoH, 5);

            dfg1 += (1 - f) * gv;
            dfg2 += f * gv;
        }
            
        // diffuse term
        Xi = frac(Xi + 0.5f);
        float pdf;
        ImportanceSampleCosDir(Xi, N, L, NoL, pdf);
        if (NoL > 0.0)
        {
            float DiffuseLoH = saturate(dot(L, normalize(V+L)));
            float DiffuseNoV = saturate(dot(N, V));
            diffuse += Fr_DisneyDiffuse(DiffuseNoV, NoL, DiffuseLoH, sqrt(roughness));
        }
    }

    lutTexture[dispatchThreadID.xy] = float4(dfg1 * 4, dfg2 * 4, diffuse, 1.f) * invNumSample;
}
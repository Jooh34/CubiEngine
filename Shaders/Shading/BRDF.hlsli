// clang-format off
#pragma once

#include "Utils.hlsli"

struct BxDFContext
{
    float VoH;
    float NoV;
    float NoL;
    float NoH;
};

float3 ComputeF0(float specular, float3 albedo, float metalic)
{
    return lerp(specular.xxx, albedo.xyz, metalic);
}

float3 diffuseLambert(float3 diffuseColor)
{
    return diffuseColor * ( 1 / PI );
}

float3 F_Schlick(float3 f0, float u)
{
    float f = pow(1.0 - u, 5.0);
    return f + f0 * (1.0 - f);
}

// https://google.github.io/filament/Filament.md.html#materialsystem/diffusebrdf
float F_Schlick(float u, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}


float3 fresnelSchlickFunctionRoughness(const float3 f0, const float vDotN, const float roughnessFactor)
{
    return f0 + (max(float3(1.0 - roughnessFactor, 1.0 - roughnessFactor, 1.0 - roughnessFactor), f0) - f0) *
                    pow(1.0 - vDotN, 5.0);
}

float3 diffusekD(float3 albedo, float VoH, float metalic)
{
    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.xyz, metalic);

    // Using cook torrance BRDF for specular lighting.
    const float3 fresnel = F_Schlick(f0, VoH);

    // Metals have kD as 0.0f, so more metalic a surface is, closes kS ~ 1 and kD ~ 0.
    // Using lambertian model for diffuse light now.
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - fresnel, float3(0.0f, 0.0f, 0.0f), metalic);

    return kD;
}
// BRDF = kD * diffuseBRDF + kS * specularBRDF. (Note : kS + kD = 1).
float3 lambertianDiffuseBRDF(const float3 albedo, const float VoH, float metalic)
{
    float3 kD = diffusekD(albedo, VoH, metalic); 
    const float3 diffuseBRDF = diffuseLambert(albedo);
    
    return kD * diffuseBRDF;
}

// https://google.github.io/filament/Filament.md.html#materialsystem/diffusebrdf
float3 Fd_Burley(float NoV, float NoL, float LoH, float roughness, float3 albedo, float VoH, float metalic) {
    float3 kD = diffusekD(albedo, VoH, metalic); 

    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return kD * albedo * lightScatter * viewScatter * (1.0 / PI);
}

// From UnrealEngine
float D_GGX(float a2, float NoH)
{
	float d = ( NoH * a2 - NoH ) * NoH + 1;	// 2 mad
	return a2 / ( PI*d*d );					// 4 mul, 1 rcp
}

float G_GeometricAttenuation(BxDFContext context)
{
    float c1 = 2 * context.NoH * context.NoV / (context.VoH+EPSILON);
    float c2 = 2 * context.NoH * context.NoL / (context.VoH+EPSILON);
    return min(min(1.f, c1),c2);
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox( float a, float NoV, float NoL )
{
	float Vis_SmithV = NoL * ( NoV * ( 1.0 - a ) + a );
	float Vis_SmithL = NoV * ( NoL * ( 1.0 - a ) + a );
	return 0.5 / ( Vis_SmithV + Vis_SmithL);
}

// From the filament docs. Geometric Shadowing function
// https://google.github.io/filament/Filament.html#toc4.4.2
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = pow(roughness, 4.0);
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL + EPS);
}

float3 CookTorrenceSpecular(float roughness, float metalic, float3 F0, BxDFContext context)
{
    float a = roughness*roughness;
    float a2 = a*a;
    
    float3 F = F_Schlick(F0, context.VoH);
    float D = D_GGX(a2, context.NoH);
    float V = V_SmithGGXCorrelated(context.NoV, context.NoL, roughness);
    // float V = Vis_SmithJointApprox(a, context.NoV, context.NoL);

    float3 color = (D*V)*F;
    return color;
}
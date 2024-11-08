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

float3 F_Schlick(const float VoH, const float3 f0)
{
    return f0 + (1.0f - f0) * pow(clamp(1.0f - VoH, 0.0f, 1.0f), 5.0f);
}

float D_GGX(float a2, float NoH)
{
	float d = ( NoH * a2 - NoH ) * NoH + 1;	// 2 mad
	return a2 / ( PI*d*d );					// 4 mul, 1 rcp
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox( float a2, float NoV, float NoL )
{
	float a = sqrt(a2);
	float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
	float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
	return 0.5 * rcp( Vis_SmithV + Vis_SmithL );
}

float3 cookTorrence(float3 albedo, float roughness, float metalic, BxDFContext context)
{
    float3 F0 = ComputeF0(0.04f, albedo, metalic);
    float a2 = pow(roughness, 4);

    float3 F = F_Schlick(context.VoH, F0);
    float D = D_GGX(a2, context.NoH);
    // float D = 1;
    float Vis = Vis_SmithJointApprox(a2, context.NoV, context.NoL);
    // float Vis = 1;
    
    // Calculation of analytical lighting contribution
    float3 diffuseContrib = (1.0 - F) * diffuseLambert(albedo);
    float3 specContrib = F * Vis * D;

    // return (D * Vis) * F / max(4.f * context.NoV * context.NoL, EPSILON);
    return context.NoL * (diffuseContrib + specContrib);
}
float3 specularGGX(float3 albedo, float roughness, float metalic, BxDFContext context)
{
    float3 F0 = ComputeF0(0.04f, albedo, metalic);
    float a2 = pow(roughness, 4);

    float3 F = F_Schlick(context.VoH, F0);
    // float D = D_GGX(a2, context.NoH);
    float D = 1;
    // float Vis = Vis_SmithJointApprox(a2, context.NoV, context.NoL);
    float Vis = 1;

    return (D * Vis) * F / max(4.f * context.NoV * context.NoL, EPSILON);
}
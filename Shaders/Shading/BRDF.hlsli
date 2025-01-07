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

// From UnrealnEngine
// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 F_Schlick(float3 SpecularColor, const float VoH )
{
	float Fc = pow( 1 - VoH, 5 );					// 1 sub, 3 mul
	//return Fc + (1 - Fc) * SpecularColor;		// 1 add, 3 mad
	
	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	return saturate( 50.0 * SpecularColor.g ) * Fc + (1 - Fc) * SpecularColor;
}

// https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float3 F_Schlick(float3 f0, float f90 , float u)
{
    return f0 + ( f90 - f0 ) * pow (1.f-u , 5.f);
}


float3 fresnelSchlickFunctionRoughness(const float3 f0, const float vDotN, const float roughnessFactor)
{
    return f0 + (max(float3(1.0 - roughnessFactor, 1.0 - roughnessFactor, 1.0 - roughnessFactor), f0) - f0) *
                    pow(1.0 - vDotN, 5.0);
}

// https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = lerp(0, 0.5, linearRoughness);
    float energyFactor = lerp(1.0, 1.0/1.51, linearRoughness);
    float fd90 = energyBias + 2.0 * LdotH*LdotH*linearRoughness;
    float3 f0 = float3(1.f, 1.f, 1.f);
    float lightScatter = F_Schlick(f0, fd90, NdotL).r;
    float viewScatter = F_Schlick(f0, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor;
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

// from UnrealEngine
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
    float3 F0 = ComputeF0(0.08f, albedo, metalic);
    float a2 = pow4(roughness);

    float3 F = F_Schlick(F0, context.VoH);
    float D = D_GGX(a2, context.NoH);
    float G = G_GeometricAttenuation(context);
    //float G = Vis_SmithJointApprox(a2, context.NoV, context.NoL); // need energy conservation?
    
    // Calculation of analytical lighting contribution
    float3 diffuseContrib = (1.0 - metalic) * (1.0 - F) * diffuseLambert(albedo);
    float3 specContrib = F * G * D / max(4 * context.NoL * context.NoV, MIN_FLOAT_VALUE);

    return context.NoL * (diffuseContrib + specContrib);
}
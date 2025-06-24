// clang-format off
#pragma once

#include "Utils.hlsli"
#include "Shading/BRDF.hlsli"

float3 ConcentricDiskSamplingHelper(float2 E)
{
	float2 p = 2 * E - 0.99999994;
	float2 a = abs(p);
	float Lo = min(a.x, a.y);
	float Hi = max(a.x, a.y);
	float Epsilon = 5.42101086243e-20; 
	float Phi = (PI / 4) * (Lo / (Hi + Epsilon) + 2 * float(a.y >= a.x));
	float Radius = Hi;
	const uint SignMask = 0x80000000;
	float2 Disk = asfloat((asuint(float2(cos(Phi), sin(Phi))) & ~SignMask) | (asuint(p) & SignMask));
	return float3(Disk, Radius);
}

float4 CosineSampleHemisphereConcentric(float2 E)
{
	float3 Result = ConcentricDiskSamplingHelper(E);
	float SinTheta = Result.z;
	float CosTheta = sqrt(1 - SinTheta * SinTheta);
	return float4(Result.xy * SinTheta, CosTheta, CosTheta * (1.0 / PI));
}


// UnrealEngine 4 by Karis
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.y;
    float CosTheta = sqrt((1-Xi.x) / (1.0+(a*a-1.0) * Xi.x));
    float SinTheta = sqrt(1-CosTheta*CosTheta);

    float3 H;
    H.x = SinTheta*cos(Phi);
    H.y = SinTheta*sin(Phi);
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = normalize(cross(N, TangentX));
    
    // Tangent to world space transformation.
    return normalize(TangentX * H.x + TangentY * H.y + N * H.z);
}

void ImportanceSampleCosDir(float2 u, float3 N, out float3 outL, out float NdotL, out float pdf)
{
    const float CosTheta = sqrt(u.x);
    const float SinTheta = sqrt(max(0.0f, 1.0f - CosTheta * CosTheta)); // Sin Theta
    float Phi = u.y * PI * 2;
    
    float3 L;
    L.x = SinTheta*cos(Phi);
    L.y = SinTheta*sin(Phi);
    L.z = CosTheta;
    
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = normalize(cross(N, TangentX));
    
    outL = normalize(TangentX * L.x + TangentY * L.y + N * L.z);

    NdotL = saturate(dot(N,L));
    pdf = max(CosTheta * INV_PI, VERY_SMALL_NUMBER);
}

// https://ameye.dev/notes/sampling-the-hemisphere/
void ImportanceSampleCosDirPow(float2 u, float3 N, float p, out float3 outL, out float NdotL, out float pdf)
{
    float CosTheta = pow(max(0.0f, 1.0f - u.x), 1.f/(p+1));
    float SinTheta = sqrt(u.x);
    float Phi = u.y * PI * 2;
    
    float3 L;
    L.x = SinTheta*cos(Phi);
    L.y = SinTheta*sin(Phi);
    L.z = CosTheta;
    
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = normalize(cross(N, TangentX));
    
    outL = normalize(TangentX * L.x + TangentY * L.y + N * L.z);

    NdotL = saturate(dot(N,L));
    pdf = max((p+1) * pow(CosTheta, p) * INV_PI / 2, VERY_SMALL_NUMBER);
}

// for normal (0,0,1)
float3 UniformSampleHemisphere(float2 uv)
{
    const float cosTheta = 1-uv.x;
    const float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta)); // Sin Theta
    const float phi = 2.0f * PI * uv.y;
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

void ConcentricSampleDisk(float2 u, float3 N, out float3 outL, out float pdf)
{
    float sx = 2.0 * u.x - 1.0;
    float sy = 2.0 * u.y - 1.0;

    if (sx == 0 && sy == 0)
        sx = 0.01f;

    float r, theta;

    if (abs(sx) > abs(sy)) {
        r = sx;
        theta = (PI / 4.0) * (sy / sx);
    } else {
        r = sy;
        theta = (PI / 2.0) - (PI / 4.0) * (sx / sy);
    }

    float cosTheta = cos(theta);

    float x = r * cosTheta;
    float y = r * sin(theta);
    float z = sqrt(saturate(1.0 - x * x - y * y));
    float3 L = float3(x, y, z);
    
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = normalize(cross(N, TangentX));
    
    outL = normalize(TangentX * L.x + TangentY * L.y + N * L.z);

    pdf = max(cosTheta * (1.f/PI), VERY_SMALL_NUMBER);
}

float3 Fresnel(in float3 specAlbedo, in float3 h, in float3 l)
{
    float3 fresnel = specAlbedo + (1.0f - specAlbedo) * pow((1.0f - saturate(dot(l, h))), 5.0f);

    // // Fade out spec entirely when lower than 0.1% albedo
    // fresnel *= saturate(dot(specAlbedo, 333.0f));

    return fresnel;
}

float SmithGGXMasking(float3 n, float3 l, float3 v, float a2)
{
    float dotNL = saturate(dot(n, l));
    float dotNV = saturate(dot(n, v));
    float denomC = sqrt(a2 + (1.0f - a2) * dotNV * dotNV) + dotNV;

    return 2.0f * dotNV / max(EPS,denomC);
}

float SmithGGXMaskingShadowing(float3 n, float3 l, float3 v, float a2)
{
    float dotNL = saturate(dot(n, l));
    float dotNV = saturate(dot(n, v));

    float denomA = dotNV * sqrt(a2 + (1.0f - a2) * dotNL * dotNL);
    float denomB = dotNL * sqrt(a2 + (1.0f - a2) * dotNV * dotNV);

    return 2.0f * dotNL * dotNV / max(EPS,denomA + denomB);
}

float3 SampleGGXVisibleNormal(float3 wo, float ax, float ay, float u1, float u2)
{
    // Stretch the view vector so we are sampling as though
    // roughness==1
    float3 v = normalize(float3(wo.x * ax, wo.y * ay, wo.z));

    // Build an orthonormal basis with v, t1, and t2
    float3 t1 = (v.z < 0.999f) ? normalize(cross(v, float3(0, 0, 1))) : float3(1, 0, 0);
    float3 t2 = cross(t1, v);

    // Choose a point on a disk with each half of the disk weighted
    // proportionally to its projection onto direction v
    float a = 1.0f / (1.0f + v.z);
    float r = sqrt(u1);
    float phi = (u2 < a) ? (u2 / a) * PI : PI + (u2 - a) / (1.0f - a) * PI;
    float p1 = r * cos(phi);
    float p2 = r * sin(phi) * ((u2 < a) ? 1.0f : v.z);

    // Calculate the normal in this stretched tangent space
    float3 n = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

    // Unstretch and normalize the normal
    return normalize(float3(ax * n.x, ay * n.y, max(0.0f, n.z)));
}

float3 ImportanceSampleGGX_V2(float3 V, float3 N, float roughness, float u1, float u2, float3 F0, float3 energyCompensation, out float3 throughput)
{
    float3 microfacetNormal = SampleGGXVisibleNormal(V, roughness, roughness, u1, u2);
    float3 L = reflect(-V, microfacetNormal);
    
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float VoH = saturate(dot(V, microfacetNormal));

    if (NoL <= EPS || NoV <= EPS || L.z < 0.0)
    {
        // If the sample direction is below the surface, we discard it.
        throughput = float3(0.0f, 0.0f, 0.0f);
        return float3(0.0f, 0.0f, 1.0f);
    }

    float LoH = saturate(dot(L, microfacetNormal));
    float3 F = F_Schlick(F0, LoH);
    float G1 = SmithGGXMasking(N, L, V, roughness * roughness);
    float G2 = SmithGGXMaskingShadowing(N, L, V, roughness * roughness);

    throughput = (F * energyCompensation) * (G2 / max(G1, EPS)); 
    return L;
}

float3 ImportanceSampleGGX_V3(float3 V, float3 N, float roughness, float u1, float u2, float3 F0, float3 energyCompensation, out float3 throughput)
{
    float3 microfacetNormal = SampleGGXVisibleNormal(V, roughness, roughness, u1, u2);
    float3 L = reflect(-V, microfacetNormal);
    
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float LoH = saturate(dot(L, microfacetNormal));
    float NoH = saturate(dot(N, microfacetNormal));

    if (NoL <= 0.0f || NoV <= 0.0f || L.z < 0.0)
    {
        // If the sample direction is below the surface, we discard it.
        throughput = float3(0.0f, 0.0f, 0.0f);
        return float3(0.0f, 0.0f, 1.0f);
    }

    float3 F = F_Schlick(F0, LoH);
    float G = SmithGGXMaskingShadowing(N, L, V, roughness * roughness);
    float weight = LoH / (NoL*NoH);

    throughput = (F * G * weight);
    return L;
}
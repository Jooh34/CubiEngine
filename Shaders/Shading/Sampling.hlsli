// clang-format off
#include "Utils.hlsli"

float3 Fresnel(in float3 specAlbedo, in float3 h, in float3 l)
{
    float3 fresnel = specAlbedo + (1.0f - specAlbedo) * pow((1.0f - saturate(dot(l, h))), 5.0f);

    // Fade out spec entirely when lower than 0.1% albedo
    fresnel *= saturate(dot(specAlbedo, 333.0f));

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
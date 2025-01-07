// clang-format off
#pragma once

static const float MIN_FLOAT_VALUE = 1.0e-5;
static const float EPSILON = 1.0e-4;
static const float EPS = 1.0e-4;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;
static const float INVALID_INDEX = 4294967295; // UINT32_MAX;

float4 getAlbedo(const float2 textureCoords, const uint albedoTextureIndex, const uint albedoTextureSamplerIndex, const float3 albedoColor)
{
    if (albedoTextureIndex == INVALID_INDEX)
    {
        return float4(albedoColor, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];

    SamplerState samplerState = SamplerDescriptorHeap[albedoTextureSamplerIndex];
    return albedoTexture.Sample(samplerState, textureCoords);
}


float3 getNormal(float2 textureCoord, uint normalTextureIndex, uint normalTextureSamplerIndex, float3 normal, float3x3 tbnMatrix)
{
    if (normalTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(normalTextureSamplerIndex)];

        // Make the normal into a -1 to 1 range.
        normal = 2.0f * normalTexture.Sample(samplerState, textureCoord).xyz - float3(1.0f, 1.0f, 1.0f);
        normal = normalize(mul(normal, tbnMatrix));
        return normal;
    }

    return normalize(normal);
}

float3 getEmissive(float2 textureCoord, float3 albedoColor, float emissiveFactor, uint emissiveTextureIndex, uint emissiveTextureSamplerIndex)
{
    if (emissiveTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[NonUniformResourceIndex(emissiveTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(emissiveTextureSamplerIndex)];

        return emissiveTexture.Sample(samplerState, textureCoord).xyz * emissiveFactor;
    }

    return albedoColor * emissiveFactor;
}

float getAO(float2 textureCoord, uint aoTextureIndex, uint aoTextureSamplerIndex)
{
    if (aoTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> aoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(aoTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(aoTextureSamplerIndex)];

        return aoTexture.Sample(samplerState, textureCoord).x;
    }

    return 1.0f;
}

float2 getMetalRoughness(float2 textureCoord, uint metalRoughnessTextureIndex, uint metalRoughnessTextureSamplerIndex)
{
    if (metalRoughnessTextureIndex != INVALID_INDEX)
    {
        Texture2D<float4> metalRoughnessTexture =
            ResourceDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureIndex)];

        SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureSamplerIndex)];

        return metalRoughnessTexture.Sample(samplerState, textureCoord).bg;
    }

    return float2(0.5f, 0.5f);
}

float3 getSamplingVector(float2 pixelCoords, uint3 dispatchThreadID)
{
    // Convert pixelCoords into the range of -1 .. 1 and make sure y goes from top to bottom.
    float2 uv = 2.0f * float2(pixelCoords.x, 1.0f - pixelCoords.y) - float2(1.0f, 1.0f);

    float3 samplingVector = float3(0.0f, 0.0f, 0.0f);

    // Based on cube face 'index', choose a vector.
    switch (dispatchThreadID.z)
    {
    case 0:
        samplingVector = float3(1.0, uv.y, -uv.x);
        break;
    case 1:
        samplingVector = float3(-1.0, uv.y, uv.x);
        break;
    case 2:
        samplingVector = float3(uv.x, 1.0, -uv.y);
        break;
    case 3:
        samplingVector = float3(uv.x, -1.0, uv.y);
        break;
    case 4:
        samplingVector = float3(uv.x, uv.y, 1.0);
        break;
    case 5:
        samplingVector = float3(-uv.x, uv.y, -1.0);
        break;
    }

    return normalize(samplingVector);
}

float4 generateTangent(float3 normal)
{
    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0));
    tangent =
        normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    return float4(tangent, 1.0f);
}

// Given 3 vectors, compute a orthonormal basis using them.
// Will be used for diffuse irradiance cube map calculation to create a set of basis vectors
// to convert from the shading / tangent space to world space.
// Reference :
// https://github.com/Nadrin/PBR/blob/cd61a5d59baa15413c7b0aff4a7da5ed9cc57f61/data/shaders/hlsl/irmap.hlsl#L73
void computeBasisVectors(float3 normal, out float3 s, out float3 t)
{
    t = cross(normal, float3(0.0f, 1.0f, 0.0f));
    t = lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), t, step(MIN_FLOAT_VALUE, dot(t, t)));

    t = normalize(t);
    s = normalize(cross(normal, t));
}

// Algorithm taken from here : https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/.
// Aims to reconstruct the view space 3D position using the inverse projection matrix, screen space uv texture coords,
// and the depth buffer.
float3 viewSpaceCoordsFromDepthBuffer(const float depthValue, const float2 uvCoords, const float4x4 inverseProjectionMatrix)
{
    float2 ndc = float2(uvCoords.x, 1.0f - uvCoords.y) * 2.0f - 1.0f;
   
    float4 unprojectedPosition = mul(float4(ndc, depthValue, 1.0f), inverseProjectionMatrix);
    return unprojectedPosition.xyz / unprojectedPosition.w;
}

void packGBuffer(const float3 albedo, const float3 normal, const float ao, float2 metalRoughness, const float3 emissive,
    out float4 GBufferA, out float4 GBufferB, out float4 GBufferC)
{
    GBufferA = float4(albedo, emissive.r);
    GBufferB = float4(normal, emissive.g);
    GBufferC = float4(ao, metalRoughness, emissive.b);
}

float pow4(float x)
{
    float xx = x*x;
    return xx*xx;
}

float vanDerCorputRadicalInverse(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, float invSample)
{
    return float2(i*invSample, vanDerCorputRadicalInverse(i));
}

// UnrealEngine 4 by Karis
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1-Xi.y) / (1+(a*a-1) * Xi.y));
    float SinTheta = sqrt(1-CosTheta*CosTheta);

    float3 H;
    H.x = SinTheta*cos(Phi);
    H.y = SinTheta*sin(Phi);
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = normalize(cross(N, TangentX));
    
    // Tangent to world space transformation.
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

// from "course notes moving frostbite to pbr"
// https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
void ImportanceSampleCosDir(float2 u, float3 N, out float3 L, out float NdotL, out float pdf)
{
    float3 upVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);

    float r = u.x;
    float phi = u.y * PI * 2;

    L = float3(r*cos(phi), r*sin(phi), sqrt(max(0.0f, 1.0f-u.x)));
    L = normalize(tangentX * L.y + tangentY * L.x + N * L.z);

    NdotL = dot(L,N);
    pdf = NdotL * INV_PI;
}
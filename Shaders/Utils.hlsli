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

static const int MAX_HALTON_SEQUENCE = 16;

static const float2 HALTON_SEQUENCE[MAX_HALTON_SEQUENCE] = {
	float2( 0.5, 0.333333 ),
	float2( 0.25, 0.666667 ),
	float2( 0.75, 0.111111 ),
	float2( 0.125, 0.444444 ),
	float2( 0.625, 0.777778 ),
	float2( 0.375, 0.222222 ),
	float2( 0.875, 0.555556 ) ,
	float2( 0.0625, 0.888889 ),
	float2( 0.5625, 0.037037 ),
	float2( 0.3125, 0.37037 ),
	float2( 0.8125, 0.703704 ),
	float2( 0.1875, 0.148148 ),
	float2( 0.6875, 0.481482 ),
	float2( 0.4375, 0.814815 ),
	float2( 0.9375, 0.259259 ),
	float2( 0.03125, 0.592593 )
};


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

float2 calculateVelocity(float4 position, float4 prevPosition)
{
    float2 currentUV = position.xy / float2(position.w, position.w);
	currentUV = currentUV * 0.5f + 0.5f;
	currentUV.y = 1.f - currentUV.y;

	float2 prevUV = prevPosition.xy / float2(prevPosition.w, prevPosition.w);
	prevUV = prevUV * 0.5f + 0.5f;
	prevUV.y = 1.f - prevUV.y;

	return currentUV - prevUV;
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

void packGBuffer(const float3 albedo, const float3 normal, const float ao, float2 metalRoughness, const float3 emissive, const float2 velocity,
    out float4 GBufferA, out float4 GBufferB, out float4 GBufferC, out float2 Velocity)
{
    GBufferA = float4(albedo, emissive.r);
    GBufferB = float4(normal, emissive.g);
    GBufferC = float4(ao, metalRoughness, emissive.b);
    Velocity = velocity; 
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
    float CosTheta = sqrt(max(0.0f, 1.0f - u.x));
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
    pdf = CosTheta * INV_PI;
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
    pdf = (p+1) * pow(CosTheta, p) * INV_PI / 2;
}

// for normal (0,0,1)
float3 UniformSampleHemisphere(float2 uv)
{
    const float cosTheta = uv.x; // cosTheta = (1-uv.x),  but okay.
    const float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta)); // Sin Theta
    const float phi = 2.0f * PI * uv.y;
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Bring the vector that was randomly sampled from a hemisphere into the coordinate system where N, S and T form the orthonormal basis.
float3 tangentToWorldCoords(float3 v, float3 n, float3 s, float3 t)
{
    return s * v.x + t * v.y + n * v.z;
}

float4 ApplyTAAJittering(float4 clipspacePosition, uint FrameCount, float2 ViewportDimensions)
{
	int idx = FrameCount % MAX_HALTON_SEQUENCE;

	float2 jitter = HALTON_SEQUENCE[idx];
	jitter.x = ( jitter.x - 0.5f ) / ViewportDimensions.x * 2.f;
	jitter.y = ( jitter.y - 0.5f ) / ViewportDimensions.y * 2.f;

	clipspacePosition.xy += jitter * clipspacePosition.w;
	return clipspacePosition;
}
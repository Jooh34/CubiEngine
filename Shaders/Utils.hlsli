// clang-format off
#pragma once

static const float MIN_FLOAT_VALUE = 1.0e-5;
static const float EPSILON = 1.0e-4;
static const float EPS = 1.0e-4;
static const float VERY_SMALL_NUMBER = 1.0e-4;

static const float PI = 3.14159265359;
static const float TWO_PI = 2.0f * PI;
static const float INV_PI = 1.0f / PI;
static const float INV_TWO_PI = 1.0f / TWO_PI;
static const uint INVALID_INDEX = 4294967295; // UINT32_MAX;

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

float4 getAlbedo(const float2 textureCoords, const uint albedoTextureIndex, const uint albedoTextureSamplerIndex, const float3 defaultAlbedoColor)
{
    if (albedoTextureIndex == INVALID_INDEX)
    {
        return float4(defaultAlbedoColor, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];
    SamplerState samplerState = SamplerDescriptorHeap[albedoTextureSamplerIndex];

    return albedoTexture.Sample(samplerState, textureCoords);
}


float3 getNormal(float2 textureCoord, uint normalTextureIndex, uint normalTextureSamplerIndex, float3 defaultNormal, float3x3 tbnMatrix)
{
    if (normalTextureIndex == INVALID_INDEX)
    {
        return normalize(defaultNormal);
    }

    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(normalTextureSamplerIndex)];

    // Make the normal into a -1 to 1 range.
    float3 normal = 2.0f * normalTexture.Sample(samplerState, textureCoord).xyz - float3(1.0f, 1.0f, 1.0f);
    normal = normalize(mul(normal, tbnMatrix));
    return normal;
}

float3 getEmissive(float2 textureCoord, uint emissiveTextureIndex, uint emissiveTextureSamplerIndex)
{
    if (emissiveTextureIndex == INVALID_INDEX)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[NonUniformResourceIndex(emissiveTextureIndex)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(emissiveTextureSamplerIndex)];

    return emissiveTexture.Sample(samplerState, textureCoord).xyz;
}

float sampleAO(float2 textureCoord, uint aoTextureIndex, uint aoTextureSamplerIndex)
{
    Texture2D<float4> aoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(aoTextureIndex)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(aoTextureSamplerIndex)];

    return aoTexture.Sample(samplerState, textureCoord).x;
}

float2 sampleMetallicRoughness(float2 textureCoord, uint metalRoughnessTextureIndex, uint metalRoughnessTextureSamplerIndex)
{
    Texture2D<float4> metalRoughnessTexture = ResourceDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureIndex)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(metalRoughnessTextureSamplerIndex)];

    return metalRoughnessTexture.Sample(samplerState, textureCoord).xy;
}

float3 sampleORMTexture(float2 textureCoord, uint ormTextureIndex, uint ormTextureSamplerIndex)
{
    Texture2D<float4> ormTexture = ResourceDescriptorHeap[NonUniformResourceIndex(ormTextureIndex)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(ormTextureSamplerIndex)];

    return ormTexture.Sample(samplerState, textureCoord).xyz;
}

float3 getOcclusionRoughnessMetallic(
    float2 textureCoord, float2 defaultMetalRoughnessValue,
    uint ormTextureIndex, uint ormTextureSamplerIndex,
    uint metalRoughnessTextureIndex, float metalRoughnessSamplerIndex,
    uint aoTextureIndex, uint aoTextureSamplerIndex
)
{
    float occlusion = 1.f; 
    float roughness = defaultMetalRoughnessValue.y;
    float metallic = defaultMetalRoughnessValue.x;

    if (ormTextureIndex != INVALID_INDEX)
    {
        float3 ORM = sampleORMTexture(textureCoord, ormTextureIndex, ormTextureSamplerIndex);
        occlusion = ORM.x;
        roughness = ORM.y;
        metallic = ORM.z;
    }
    else if (metalRoughnessTextureIndex != INVALID_INDEX)
    {
        float2 metallicRoughness = sampleMetallicRoughness(textureCoord, metalRoughnessTextureIndex, metalRoughnessSamplerIndex);
        roughness = metallicRoughness.y;
        metallic = metallicRoughness.x;
    }

    if (ormTextureIndex != INVALID_INDEX)
    {
        occlusion = sampleAO(textureCoord, aoTextureIndex, aoTextureSamplerIndex);
    }

    return float3(occlusion, roughness, metallic);
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

// UE5
float3x3 GetTangentBasis( float3 TangentZ )
{
	const float Sign = TangentZ.z >= 0 ? 1 : -1;
	const float a = -rcp( Sign + TangentZ.z );
	const float b = TangentZ.x * TangentZ.y * a;
	float3 TangentX = { 1 + Sign * a * pow(2, TangentZ.x ), Sign * b, -Sign * TangentZ.x };
	float3 TangentY = { b,  Sign + a * pow(2, TangentZ.y ), -TangentZ.y };
	return float3x3( TangentX, TangentY, TangentZ );
}


// Bring the vector that was randomly sampled from a hemisphere into the coordinate system where N, S and T form the orthonormal basis.
float3 tangentToWorldCoords(float3 v, float3 n, float3 s, float3 t)
{
    return s * v.x + t * v.y + n * v.z;
}

// not used currently.
float4 ApplyTAAJittering(float4 clipspacePosition, uint FrameCount, float2 ViewportDimensions)
{
	int idx = FrameCount % MAX_HALTON_SEQUENCE;

	float2 jitter = HALTON_SEQUENCE[idx];
	jitter.x = ( jitter.x - 0.5f ) / ViewportDimensions.x * 2.f;
	jitter.y = ( jitter.y - 0.5f ) / ViewportDimensions.y * 2.f;

	clipspacePosition.xy += jitter * clipspacePosition.w;
	return clipspacePosition;
}

float InterleavedGradientNoise(float2 uv, int FrameCount){
	// magic values are found by experimentation
    FrameCount = FrameCount%8;
	uv += float(FrameCount) * (float2(47, 17) * 0.695f);

    float3 magic = float3( 0.06711056f, 0.00583715f, 52.9829189f );
    
    return frac(magic.z * frac(dot(uv, magic.xy)));
} 

float InterleavedGradientNoiseByIntel(int2 pixelPos, int frameIndex = 0)
{
    // 상수는 적절한 해싱을 위한 마법 숫자들
    uint x = uint(pixelPos.x);
    uint y = uint(pixelPos.y);
    uint f = uint(frameIndex);

    uint m = x + y * 374761393u + f * 668265263u;
    m = (m ^ (m >> 13u)) * 1274126177u;
    return frac(float((m ^ (m >> 16u)) & 0x00FFFFFFu) / 16777216.0);
}

// @param DeviceZ value that is stored in the depth buffer (Z/W)
// @return SceneDepth (linear in world units, W)
float ConvertFromDeviceZ(float DeviceZ, float4 InvDeviceZToWorldZTransform)
{
	// Supports ortho and perspective, see CreateInvDeviceZToWorldZTransform()
	return DeviceZ * InvDeviceZToWorldZTransform[0] + InvDeviceZToWorldZTransform[1] + 1.0f / (DeviceZ * InvDeviceZToWorldZTransform[2] - InvDeviceZToWorldZTransform[3]);
}

float2 ClipToUV(float2 ClipXY)
{
    float2 uv = ClipXY*0.5f+0.5f;
    uv.y = 1.f - uv.y;
    return uv;
}

float2 UVToClip(float2 UV)
{
    float2 clip = float2(UV.x, 1.0f - UV.y) * 2.0f - 1.0f;
    return clip;
}

inline float pow2(float x) {
    return pow(x, 2.0);
}


float3x3 Inverse3x3(float3x3 m)
{
    float a00 = m[0].x, a01 = m[0].y, a02 = m[0].z;
    float a10 = m[1].x, a11 = m[1].y, a12 = m[1].z;
    float a20 = m[2].x, a21 = m[2].y, a22 = m[2].z;

    float det =
        a00 * (a11 * a22 - a12 * a21) -
        a01 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * a21 - a11 * a20);

    if (abs(det) < 1e-6)
        return float3x3(0,0,0, 0,0,0, 0,0,0); // 혹은 fallback 처리

    float invDet = 1.0 / det;

    return float3x3(
        (a11 * a22 - a12 * a21) * invDet,
        (a02 * a21 - a01 * a22) * invDet,
        (a01 * a12 - a02 * a11) * invDet,

        (a12 * a20 - a10 * a22) * invDet,
        (a00 * a22 - a02 * a20) * invDet,
        (a02 * a10 - a00 * a12) * invDet,

        (a10 * a21 - a11 * a20) * invDet,
        (a01 * a20 - a00 * a21) * invDet,
        (a00 * a11 - a01 * a10) * invDet
    );
}
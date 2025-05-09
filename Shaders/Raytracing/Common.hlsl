#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

static const float FP32Max = 3.402823466e+38f;

// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct FPayload
{
  float3 radiance;
  float distance;
};

struct FShadowPayload
{
    float visibility;
};

enum RayTypes {
    RayTypeRadiance = 0,
    RayTypeShadow = 1,

    NumRayTypes
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 barycentrics : SV_Barycentrics;
};


float BarycentricLerp(in float v0, in float v1, in float v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float2 BarycentricLerp(in float2 v0, in float2 v1, in float2 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 BarycentricLerp(in float3 v0, in float3 v1, in float3 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float4 BarycentricLerp(in float4 v0, in float4 v1, in float4 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

interlop::MeshVertex BarycentricLerp(in interlop::MeshVertex v0, in interlop::MeshVertex v1, in interlop::MeshVertex v2, in float3 barycentrics)
{
    interlop::MeshVertex vtx;
    vtx.position = BarycentricLerp(v0.position, v1.position, v2.position, barycentrics);
    vtx.normal = normalize(BarycentricLerp(v0.normal, v1.normal, v2.normal, barycentrics));
    vtx.texcoord = BarycentricLerp(v0.texcoord, v1.texcoord, v2.texcoord, barycentrics);
    vtx.tangent = normalize(BarycentricLerp(v0.tangent, v1.tangent, v2.tangent, barycentrics));
    // vtx.Bitangent = normalize(BarycentricLerp(v0.Bitangent, v1.Bitangent, v2.Bitangent, barycentrics));

    return vtx;
}

// Loops up the vertex data for the hit triangle and interpolates its attributes
interlop::MeshVertex GetHitSurface(in Attributes attr, in interlop::RTSceneDebugRenderResource renderResources, in uint geometryIdx)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    StructuredBuffer<interlop::FRaytracingGeometryInfo> geoInfoBuffer = ResourceDescriptorHeap[renderResources.geometryInfoBufferIdx];
    const interlop::FRaytracingGeometryInfo geoInfo = geoInfoBuffer[geometryIdx];

    StructuredBuffer<interlop::MeshVertex> vtxBuffer = ResourceDescriptorHeap[renderResources.vtxBufferIdx];
    StructuredBuffer<uint> idxBuffer = ResourceDescriptorHeap[renderResources.idxBufferIdx];

    const uint primIdx = PrimitiveIndex();
    const uint idx0 = idxBuffer[primIdx * 3 + geoInfo.idxOffset + 0];
    const uint idx1 = idxBuffer[primIdx * 3 + geoInfo.idxOffset + 1];
    const uint idx2 = idxBuffer[primIdx * 3 + geoInfo.idxOffset + 2];

    const interlop::MeshVertex vtx0 = vtxBuffer[idx0 + geoInfo.vtxOffset];
    const interlop::MeshVertex vtx1 = vtxBuffer[idx1 + geoInfo.vtxOffset];
    const interlop::MeshVertex vtx2 = vtxBuffer[idx2 + geoInfo.vtxOffset];

    return BarycentricLerp(vtx0, vtx1, vtx2, barycentrics);
}

interlop::FRaytracingMaterial GetGeometryMaterial(in interlop::RTSceneDebugRenderResource renderResources, in uint geometryIdx)
{
    StructuredBuffer<interlop::FRaytracingGeometryInfo> geoInfoBuffer = ResourceDescriptorHeap[renderResources.geometryInfoBufferIdx];
    const interlop::FRaytracingGeometryInfo geoInfo = geoInfoBuffer[geometryIdx];

    StructuredBuffer<interlop::FRaytracingMaterial> materialBuffer = ResourceDescriptorHeap[renderResources.materialBufferIdx];
    return materialBuffer[geoInfo.materialIdx];
}

float4 getAlbedoSample(const float2 textureCoords, const uint albedoTextureIndex, SamplerState MeshSampler, const float3 albedoColor)
{
    uint InvalidIndex = 4294967295;
    if (albedoTextureIndex == InvalidIndex)
    {
        return float4(albedoColor, 1.0f);
    }

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoTextureIndex];

    return albedoTexture.SampleLevel(MeshSampler, textureCoords, 0.f);
}


float3 getNormalSample(float2 textureCoord, uint normalTextureIndex, SamplerState MeshSampler, float3 normal, float3x3 tbnMatrix)
{
    uint InvalidIndex = 4294967295;
    if (normalTextureIndex != InvalidIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];

        // Make the normal into a -1 to 1 range.
        normal = 2.0f * normalTexture.SampleLevel(MeshSampler, textureCoord, 0).xyz - float3(1.0f, 1.0f, 1.0f);
        normal = normalize(mul(normal, tbnMatrix));
        return normal;
    }

    return normalize(normal);
}
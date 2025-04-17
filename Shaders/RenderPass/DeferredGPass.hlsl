#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
    float3x3 viewMatrix : VIEW_MATRIX;
    float3x3 tbnMatrix : TBN_MATRIX;
    float4 curPosition : CUR_POSITION;
    float4 prevPosition : PREV_POSITION;
};

ConstantBuffer<interlop::DeferredGPassRenderResources> renderResources : register(b0);

 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float3> normalBuffer = ResourceDescriptorHeap[renderResources.normalBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];
    StructuredBuffer<float3> tangentBuffer = ResourceDescriptorHeap[renderResources.tangentBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];

    const matrix mvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewProjectionMatrix);
    const float3x3 normalMatrix = (float3x3)transpose(transformBuffer.inverseModelMatrix);
    const matrix prevMvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.prevViewProjMatrix);

    VSOutput output;
    float4 clipspacePosition = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.position = clipspacePosition;
    output.curPosition = clipspacePosition;
    output.prevPosition = mul(float4(positionBuffer[vertexID], 1.0f), prevMvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];
    output.viewMatrix = (float3x3)sceneBuffer.viewMatrix;

    const float3 tangent = normalize(tangentBuffer[vertexID]);
    const float3 biTangent = normalize(cross(output.normal, tangent));
    const float3 t = normalize(mul(tangent, normalMatrix));
    const float3 b = normalize(mul(biTangent, normalMatrix));
    const float3 n = normalize(mul(output.normal, normalMatrix));

    output.tbnMatrix = float3x3(t, b, n);
    return output;
}

struct PsOutput
{
    float4 GBufferA : SV_Target0;
    float4 GBufferB : SV_Target1;
    float4 GBufferC : SV_Target2;
    float2 Velocity : SV_Target3;
};

 
PsOutput PsMain(VSOutput psInput) 
{
    ConstantBuffer<interlop::MaterialBuffer> materialBuffer = ResourceDescriptorHeap[renderResources.materialBufferIndex];

    float4 albedoEmissive = getAlbedo(psInput.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex, materialBuffer.albedoColor);
    if (albedoEmissive.a < 0.9f)
    {
        discard;
    }

    float3 albedo = albedoEmissive.xyz;
    float3 normal = getNormal(psInput.textureCoord, renderResources.normalTextureIndex, renderResources.normalTextureSamplerIndex, psInput.normal, psInput.tbnMatrix).xyz;
    normal = mul(normal, psInput.viewMatrix);
    float ao = getAO(psInput.textureCoord, renderResources.aoTextureIndex, renderResources.aoTextureSamplerIndex).x;
    float2 metalRoughness = getMetalRoughness(psInput.textureCoord, renderResources.metalRoughnessTextureIndex, renderResources.metalRoughnessTextureSamplerIndex) * float2(materialBuffer.metallicFactor, materialBuffer.roughnessFactor).xy;
    float3 emissive = getEmissive(psInput.textureCoord, albedo, materialBuffer.emissiveFactor, renderResources.emissiveTextureIndex, renderResources.emissiveTextureSamplerIndex).xyz;
    float2 velocity = calculateVelocity(psInput.curPosition, psInput.prevPosition);
    PsOutput output;
    packGBuffer(albedo, normal, ao, metalRoughness, emissive, velocity, output.GBufferA, output.GBufferB, output.GBufferC, output.Velocity);
    return output;
}
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3 normal : NORMAL;
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

    const matrix mvpMatrix = mul(renderResources.modelMatrix, sceneBuffer.viewProjectionMatrix);
    const float3x3 normalMatrix = (float3x3)transpose(renderResources.inverseModelMatrix);
    const matrix prevMvpMatrix = mul(renderResources.modelMatrix, sceneBuffer.prevViewProjMatrix);

    VSOutput output;
    float4 clipspacePosition = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.position = clipspacePosition;
    output.curPosition = clipspacePosition;
    output.prevPosition = mul(float4(positionBuffer[vertexID], 1.0f), prevMvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.normal = normalBuffer[vertexID];

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
    ConstantBuffer<interlop::DebugBuffer> debugBuffer = ResourceDescriptorHeap[renderResources.debugBufferIndex];
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];

    float4 albedoEmissive = getAlbedo(psInput.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex, materialBuffer.albedoColor);
    if (albedoEmissive.a < 0.9f)
    {
        discard;
    }

    float3 albedo = albedoEmissive.xyz;
    float3 normal = getNormal(psInput.textureCoord, renderResources.normalTextureIndex, renderResources.normalTextureSamplerIndex, psInput.normal, psInput.tbnMatrix).xyz;
    float3x3 viewMatrix = (float3x3)transpose(sceneBuffer.inverseViewMatrix);
    normal = mul(normal, viewMatrix);
    
    float3 emissive = getEmissive(psInput.textureCoord, renderResources.emissiveTextureIndex, renderResources.emissiveTextureSamplerIndex).xyz;
    float2 velocity = calculateVelocity(psInput.curPosition, psInput.prevPosition);
    
    float2 defaultMetalRoughness = float2(materialBuffer.metallicFactor, materialBuffer.roughnessFactor);
    float3 orm = getOcclusionRoughnessMetallic(
        psInput.textureCoord, defaultMetalRoughness,
        renderResources.ormTextureIndex, renderResources.ormTextureSamplerIndex,
        renderResources.metalRoughnessTextureIndex, renderResources.metalRoughnessTextureSamplerIndex,
        renderResources.aoTextureIndex, renderResources.aoTextureSamplerIndex
    );

    PsOutput output;
    packGBuffer(albedo, normal, orm.xyz, emissive, velocity, output.GBufferA, output.GBufferB, output.GBufferC, output.Velocity);
    return output;
}
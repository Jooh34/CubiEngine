
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEXTURE_COORD;
    float3x3 viewMatrix : VIEW_MATRIX;
};

ConstantBuffer<interlop::UnlitPassRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    StructuredBuffer<float2> textureCoordBuffer = ResourceDescriptorHeap[renderResources.textureCoordBufferIndex];

    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    ConstantBuffer<interlop::TransformBuffer> transformBuffer = ResourceDescriptorHeap[renderResources.transformBufferIndex];

    const matrix mvpMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewProjectionMatrix);
    const matrix mvMatrix = mul(transformBuffer.modelMatrix, sceneBuffer.viewMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.textureCoord = textureCoordBuffer[vertexID];
    output.viewMatrix = (float3x3)sceneBuffer.viewMatrix;

    return output;
}

struct PsOutput
{
    float4 albedo : SV_Target0;
};

[RootSignature(BindlessRootSignature)] 
PsOutput PsMain(VSOutput psInput) 
{
    ConstantBuffer<interlop::MaterialBuffer> materialBuffer = ResourceDescriptorHeap[renderResources.materialBufferIndex];

    PsOutput output;
    output.albedo = getAlbedo(psInput.textureCoord, renderResources.albedoTextureIndex, renderResources.albedoTextureSamplerIndex, materialBuffer.albedoColor);
    
    return output;
}
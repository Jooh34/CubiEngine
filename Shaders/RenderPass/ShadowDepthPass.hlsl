// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
};

ConstantBuffer<interlop::ShadowDepthPassRenderResource> renderResources : register(b0);

 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    
    const matrix mvpMatrix = mul(renderResources.modelMatrix, renderResources.lightViewProjectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    return output;
}

 
void PsMain(VSOutput input) 
{
}
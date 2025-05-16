// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float depth : DEPTH;
};

ConstantBuffer<interlop::ShadowDepthPassRenderResource> renderResources : register(b0);

 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    StructuredBuffer<float3> positionBuffer = ResourceDescriptorHeap[renderResources.positionBufferIndex];
    
    const matrix mvpMatrix = mul(renderResources.modelMatrix, renderResources.lightViewProjectionMatrix);

    VSOutput output;
    output.position = mul(float4(positionBuffer[vertexID], 1.0f), mvpMatrix);
    output.depth = output.position.z / output.position.w;
    return output;
}


struct PsOutput
{
    float2 Moment : SV_Target0;
};

float2 ComputeMoments(float Depth)
{
    float2 Moments;
    // First moment is the depth itself.
    Moments.x = Depth;
    // Compute partial derivatives of depth.
    float dx = ddx(Depth);
    float dy = ddy(Depth);
    // Compute second moment over the pixel extents.
    Moments.y = Depth * Depth - 0.25 * (dx * dx + dy * dy);
    return Moments;
}

 
PsOutput PsMain(VSOutput input) 
{
    float2 moment = ComputeMoments(input.depth);

    PsOutput output;
    output.Moment = moment;
    return output;
}
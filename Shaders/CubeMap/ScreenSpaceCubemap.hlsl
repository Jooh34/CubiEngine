
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSInput
{
    uint VertexID : SV_VertexID; // Auto-generated ID for quad vertices
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 modelSpacePosition : POSITION;
};

ConstantBuffer<interlop::ScreenSpaceCubeMapRenderResources> renderResource : register(b0);

[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResource.sceneBufferIndex];

    float2 quadVertices[6] = {
        float2(-1.0, -1.0), // Bottom-left
        float2(-1.0,  1.0), // Top-left
        float2( 1.0, -1.0), // Bottom-right

        float2( 1.0, -1.0), // Bottom-right
        float2(-1.0,  1.0), // Top-left
        float2( 1.0,  1.0), // Top-right
    };

    VSOutput output;
    float4 vertexPosition = float4(quadVertices[vertexID], 1.0, 1.0); // Z=0, W=1 for clip space
    output.position = vertexPosition;

    output.modelSpacePosition = mul(mul(vertexPosition, sceneBuffer.inverseProjectionMatrix), sceneBuffer.inverseViewMatrix);
    output.modelSpacePosition = float4(output.modelSpacePosition.xyz/output.modelSpacePosition.w, 1.f);

    return output;
}

float4 PsMain(VSOutput input): SV_Target
{
    TextureCube cubemapTexture = ResourceDescriptorHeap[renderResource.cubenmapTextureIndex];

    const float3 samplingVector = normalize(input.modelSpacePosition.xyz);

    return cubemapTexture.Sample(linearWrapSampler, samplingVector);
    //return float4(0.5f, 0.5f, 0.5f, 1.f);
}
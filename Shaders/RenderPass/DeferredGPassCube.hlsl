#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3x3 viewMatrix : VIEW_MATRIX;
    float4 curPosition : CUR_POSITION;
    float4 prevPosition : PREV_POSITION;
};

ConstantBuffer<interlop::DeferredGPassCubeRenderResources> renderResources : register(b0);

// 정점 ID를 기반으로 큐브의 정점을 생성하는 함수
void GetCubeVertex(uint vertexID, out float3 outPosition, out float3 outNormal)
{
    float halfsize = 0.5f;
    static const float3 vertices[8] =
    {
        float3(-halfsize, -halfsize, -halfsize), // 0
        float3( halfsize, -halfsize, -halfsize), // 1
        float3(-halfsize,  halfsize, -halfsize), // 2
        float3( halfsize,  halfsize, -halfsize), // 3
        float3(-halfsize, -halfsize,  halfsize), // 4
        float3( halfsize, -halfsize,  halfsize), // 5
        float3(-halfsize,  halfsize,  halfsize), // 6
        float3( halfsize,  halfsize,  halfsize)  // 7
    };

    // ClockWise
    static const uint indices[36] =
    {
        0,2,1,  1,2,3,  // 앞면
        4,5,6,  5,7,6,  // 뒷면
        0,4,2,  2,4,6,  // 왼쪽면
        1,3,5,  3,7,5,  // 오른쪽면
        2,6,3,  3,6,7,  // 위쪽면
        0,1,4,  1,5,4   // 아래쪽면
    };

    static const float3 normals[6] =
    {
        float3( 0,  0, -1),  // 앞면
        float3( 0,  0,  1),  // 뒷면
        float3(-1,  0,  0),  // 왼쪽면
        float3( 1,  0,  0),  // 오른쪽면
        float3( 0,  1,  0),  // 위쪽면
        float3( 0, -1,  0)   // 아래쪽면
    };

    uint faceID = vertexID / 6; // 정점이 속한 면 계산
    outPosition = vertices[indices[vertexID]];
    outNormal = normals[faceID];
}


[RootSignature(BindlessRootSignature)] 
VSOutput VsMain(uint vertexID : SV_VertexID) 
{
    ConstantBuffer<interlop::SceneBuffer> sceneBuffer = ResourceDescriptorHeap[renderResources.sceneBufferIndex];
    float4x4 modelMatrix = renderResources.modelMatrix;
    float4x4 inverseModelMatrix = renderResources.inverseModelMatrix;

    float3 worldPos, normal;
    GetCubeVertex(vertexID, worldPos, normal);

    const matrix mvpMatrix = mul(modelMatrix, sceneBuffer.viewProjectionMatrix);
    const float3x3 normalMatrix = (float3x3)transpose(inverseModelMatrix);
    const matrix prevMvpMatrix = mul(modelMatrix, sceneBuffer.prevViewProjMatrix);

    VSOutput output;
    float4 clipspacePosition = mul(float4(worldPos, 1.0f), mvpMatrix);
    output.position = clipspacePosition;
    output.curPosition = clipspacePosition;
    output.prevPosition = mul(float4(worldPos, 1.0f), prevMvpMatrix);

    output.normal = normal;
    output.viewMatrix = (float3x3)sceneBuffer.viewMatrix;

    return output;
}

struct PsOutput
{
    float4 GBufferA : SV_Target0;
    float4 GBufferB : SV_Target1;
    float4 GBufferC : SV_Target2;
    float2 Velocity : SV_Target3;
};

[RootSignature(BindlessRootSignature)] 
PsOutput PsMain(VSOutput psInput) 
{
    float3 lightColor = renderResources.lightColor.xyz;
    float intensity = renderResources.intensityDistance.x;
    
    float3 albedo = lightColor * intensity;
    float3 normal = psInput.normal;
    normal = mul(normal, psInput.viewMatrix);
    float ao = 1;
    float2 metalRoughness = float2(0,1);
    float3 emissive = lightColor * intensity;
    float2 velocity = calculateVelocity(psInput.curPosition, psInput.prevPosition);
    PsOutput output;
    packGBuffer(albedo, normal, ao, metalRoughness, emissive, velocity, output.GBufferA, output.GBufferB, output.GBufferC, output.Velocity);
    return output;
}
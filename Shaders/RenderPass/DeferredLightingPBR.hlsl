#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::PBRRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numThreads(8, 8, 1)]
void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> GBufferA = ResourceDescriptorHeap[renderResources.GBufferAIndex];
    Texture2D<float4> GBufferB = ResourceDescriptorHeap[renderResources.GBufferBIndex];
    Texture2D<float4> GBufferC = ResourceDescriptorHeap[renderResources.GBufferCIndex];

    float3 temp = GBufferA[dispatchThreadID.xy].xyz + GBufferB[dispatchThreadID.xy].xyz + GBufferC[dispatchThreadID.xy].xyz;
    temp = temp / 3.f;
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[renderResources.outputTextureIndex];

    outputTexture[dispatchThreadID.xy] = float4(temp, 1.0f);
}
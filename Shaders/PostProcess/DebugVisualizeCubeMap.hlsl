// clang-format off
#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"

ConstantBuffer<interlop::DebugVisualizeCubeMapRenderResources> renderResources : register(b0);

[RootSignature(BindlessRootSignature)]
[numthreads(8, 8, 1)]

void CsMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float4> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float4> dstTexture = ResourceDescriptorHeap[renderResources.dstTextureIndex];

    int targetX = renderResources.width/4;
    int targetY = renderResources.height/4; 

    float2 srcDimension = float2(0.0f, 0.0f);
    srcTexture.GetDimensions(srcDimension.x, srcDimension.y);

    dstTexture[dispatchThreadID.xy] = srcTexture[dispatchThreadID.xy];
}
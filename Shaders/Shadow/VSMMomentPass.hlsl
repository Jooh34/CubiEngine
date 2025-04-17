// clang-format off

#include "RootSignature/BindlessRS.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Utils.hlsli"

ConstantBuffer<interlop::VSMMomentPassRenderResource> renderResources : register(b0);

[numthreads(8, 8, 1)]
void CsMain( uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    Texture2D<float> srcTexture = ResourceDescriptorHeap[renderResources.srcTextureIndex];
    RWTexture2D<float2> momentTexture = ResourceDescriptorHeap[renderResources.momentTextureIndex];

    float2 texelSize = renderResources.dstTexelSize;
    float2 uvCoords = texelSize * (coord + 0.5);

    float2 moment = float2(0.f, 0.f);
    
    // // Map destination pixel to 4 source texels
    // float2 srcCoord00 = uvCoords + texelSize * float2(-0.5f, -0.5f);
    // float2 srcCoord10 = uvCoords + texelSize * float2(0.5f, -0.5f);
    // float2 srcCoord01 = uvCoords + texelSize * float2(-0.5f, 0.5f);
    // float2 srcCoord11 = uvCoords + texelSize * float2(0.5f, 0.5f);

    // // // Fetch and average
    // float d00 = srcTexture.Sample(pointClampSampler, srcCoord00);
    // float d10 = srcTexture.Sample(pointClampSampler, srcCoord10);
    // float d01 = srcTexture.Sample(pointClampSampler, srcCoord01);
    // float d11 = srcTexture.Sample(pointClampSampler, srcCoord11);
    
    float A = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(-1.f, -1.f));
    float B = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(-1.f, 0.f));
    float C = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(-1.f, 1.f));
    float D = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(0.f, 1.f));
    float E = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(1.f, 1.f));
    float F = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(1.f, 0.f));
    float G = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(1.f, -1.f));
    float H = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(0.f, -1.f));
    float I = srcTexture.Sample(pointClampSampler, uvCoords + texelSize * float2(0.f, 0.f));

    moment.x = (A+B+C+D+E+F+G+H+I) / 9.f;
    moment.y = (pow2(A)+pow2(B)+pow2(C)+pow2(D)+pow2(E)+pow2(F)+pow2(G)+pow2(H)+pow2(I)) / 9.f;
    
    momentTexture[coord] = moment;
}
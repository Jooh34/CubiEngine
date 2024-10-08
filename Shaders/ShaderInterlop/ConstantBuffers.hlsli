// clang-format off
#pragma once

// To be able to share Constant Buffer Types (Structs) between HLSL and C++, a few defines are set here.
// Define the Structs in HLSLS syntax, and with this defines the C++ application can also use the structs.
#ifdef __cplusplus
    #define float4 Dx::XMFLOAT4
    #define float3 Dx::XMFLOAT3
    #define float2 Dx::XMFLOAT2

    #define uint uint32_t

    // Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp
    // code side.
    #define float4x4 Dx::XMMATRIX

    #define ConstantBufferStruct struct alignas(256)
#else
    // if HLSL
    #define ConstantBufferStruct struct
#endif

namespace interlop
{
    ConstantBufferStruct SceneBuffer
    {
        float4x4 viewProjectionMatrix;
        float4x4 projectionMatrix;
        float4x4 inverseProjectionMatrix;
        float4x4 viewMatrix;
        float4x4 inverseViewMatrix;
    };

    ConstantBufferStruct MaterialBuffer
    {
        float3 albedoColor;
        float roughnessFactor;
        
        float metallicFactor;
        float emissiveFactor;
        float2 padding;
    };
} // namespace interlop

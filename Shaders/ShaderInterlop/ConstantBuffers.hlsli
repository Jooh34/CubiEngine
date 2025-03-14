// clang-format off
#pragma once

// To be able to share Constant Buffer Types (Structs) between HLSL and C++, a few defines are set here.
// Define the Structs in HLSLS syntax, and with this defines the C++ application can also use the structs.
#ifdef __cplusplus
    #define float4 XMFLOAT4
    #define float3 XMFLOAT3
    #define float2 XMFLOAT2

    #define uint uint32_t

    // Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp
    // code side.
    #define float4x4 XMMATRIX

    #define ConstantBufferStruct struct alignas(256)
#else
    // if HLSL
    #define ConstantBufferStruct struct
#endif

namespace interlop
{
    ConstantBufferStruct DebugBuffer
    {
        uint bUseTaa;
        uint bUseShadow;
        float2 padding;
    };

    ConstantBufferStruct SceneBuffer
    {
        float4x4 viewProjectionMatrix;
        float4x4 projectionMatrix;
        float4x4 inverseProjectionMatrix;
        float4x4 viewMatrix;
        float4x4 inverseViewMatrix;
        float4x4 prevViewProjMatrix;
        float nearZ;
        float farZ;
        uint width;
        uint height;

        uint frameCount;
        float3 padding;
    };

    ConstantBufferStruct MaterialBuffer
    {
        float3 albedoColor;
        float roughnessFactor;

        float metallicFactor;
        float emissiveFactor;
        float2 padding;
    };

    ConstantBufferStruct TransformBuffer
    {
        float4x4 modelMatrix;
        float4x4 inverseModelMatrix;
    };

    static const uint MAX_LIGHTS = 4u;

    ConstantBufferStruct LightBuffer
    {
        float4 lightPosition[MAX_LIGHTS];
        float4 lightColor[MAX_LIGHTS];
        float4 viewSpaceLightPosition[MAX_LIGHTS];
        
        // float4 because of struct packing (16byte alignment).
        // radiusIntensity[0] stores the radius, while index 1 stores the intensity.
        float4 intensityDistance[MAX_LIGHTS];
        uint numLight;
    };
    
    static const uint MAX_CASCADE = 4;
    ConstantBufferStruct ShadowBuffer
    {
        float4x4 lightViewProjectionMatrix[MAX_CASCADE];
        float4 distanceCSM;
        float shadowBias;
        float3 padding;
    };
} // namespace interlop

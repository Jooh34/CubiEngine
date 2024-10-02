// clang-format off
#pragma once

#ifdef __cplusplus
    #define uint uint32_t
    #define float2 math::XMFLOAT2
#endif

namespace interlop
{
    struct TriangleRenderResources
    {
        uint sceneBufferIndex;
        uint positionBufferIndex;
        uint textureCoordBufferIndex;
        uint textureIndex;
    };
}

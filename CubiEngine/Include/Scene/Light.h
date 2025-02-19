#pragma once
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FScene;

class FLight
{
public:
    FLight();
    void AddLight(float Position[4], float Color[4], float Intensity = 1.f, float MaxDistance = 3000.f);
    interlop::LightBuffer GetLightBufferWithViewUpdate(FScene* Scene, XMMATRIX ViewMatrix);
    interlop::ShadowBuffer GetShadowBuffer(FScene* Scene);

    interlop::LightBuffer LightBufferData;
    interlop::ShadowBuffer ShadowBufferData;
};
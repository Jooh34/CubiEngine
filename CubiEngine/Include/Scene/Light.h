#pragma once
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FLight
{
public:
    FLight();
    void AddLight(float Position[4], float Color[4], float Intensity = 1.f, float MaxDistance = 3000.f);
    interlop::LightBuffer GetLightBufferWithViewUpdate(XMMATRIX ViewMatrix);
    interlop::LightBuffer LightBufferData;
};
#pragma once
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FLight
{
public:
    FLight();
    void AddLight(float Position[4], float Color[4], float Intensity = 1.f);
    interlop::LightBuffer GetLightBufferWithViewUpdate(XMMATRIX ViewMatrix);
    interlop::LightBuffer LightBufferData;
};
#pragma once
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FLight
{
public:
    FLight();
    void AddLight(float Position[4], float Color[4]);
    interlop::LightBuffer GetLightBuffer(XMMATRIX ViewMatrix);

private:
    float LightPosition[interlop::MAX_LIGHTS][4];
    float LightColor[interlop::MAX_LIGHTS][4];

    uint32_t NumLight;
};
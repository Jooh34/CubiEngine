#include "Scene/Light.h"

FLight::FLight()
{
    LightBufferData = {
        .numLight = 0,
    };
}

void FLight::AddLight(float Position[4], float Color[4], float Intensity)
{
    if (LightBufferData.numLight >= interlop::MAX_LIGHTS)
    {
        return;
    }
    
    uint32_t CurIndex = LightBufferData.numLight;

    LightBufferData.lightPosition[CurIndex] = XMFLOAT4{
        Position[0],
        Position[1],
        Position[2],
        Position[3],
    };

    LightBufferData.lightColor[CurIndex] = XMFLOAT4{
        Color[0],
        Color[1],
        Color[2],
        Color[3],
    };

    LightBufferData.intensity[CurIndex] = Intensity;

    LightBufferData.numLight++;
}

interlop::LightBuffer FLight::GetLightBufferWithViewUpdate(XMMATRIX ViewMatrix)
{
    for (int i = 0; i < LightBufferData.numLight; i++)
    {
        XMFLOAT4 LightPositionXM = LightBufferData.lightPosition[i];

        XMVECTOR Vec = XMVector4Transform(Dx::XMLoadFloat4(&LightPositionXM), ViewMatrix);
        Dx::XMStoreFloat4(&LightBufferData.viewSpaceLightPosition[i], XMVector3Normalize(Vec));
    }

    return LightBufferData;
}

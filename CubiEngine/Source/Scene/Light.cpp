#include "Scene/Light.h"

FLight::FLight()
{
    NumLight = 0;
}

void FLight::AddLight(float Position[4], float Color[4])
{
    if (NumLight >= interlop::MAX_LIGHTS)
    {
        return;
    }

    memcpy(LightPosition[NumLight], Position, sizeof(float)*4);
    memcpy(LightColor[NumLight], Color, sizeof(float)*4);
    NumLight++;
}

interlop::LightBuffer FLight::GetLightBuffer(XMMATRIX ViewMatrix)
{
    interlop::LightBuffer LightBufferData = {
        .numLight = 0,
    };
    
    for (int i = 0; i < NumLight; i++)
    {
        XMFLOAT4 LightPositionXM = XMFLOAT4{
            LightPosition[i][0],
            LightPosition[i][1],
            LightPosition[i][2],
            LightPosition[i][3],
        };
        LightBufferData.lightPosition[i] = LightPositionXM;

        LightBufferData.lightColor[i] = XMFLOAT4{
            LightColor[i][0],
            LightColor[i][1],
            LightColor[i][2],
            LightColor[i][3],
        };
        
        XMVECTOR Vec = XMVector4Transform(Dx::XMLoadFloat4(&LightPositionXM), ViewMatrix);
        Dx::XMStoreFloat4(&LightBufferData.viewSpaceLightPosition[i], Vec);
    }

    return LightBufferData;
}

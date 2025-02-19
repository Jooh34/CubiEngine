#include "Scene/Light.h"
#include "Scene/Scene.h"

FLight::FLight()
{
    LightBufferData = {
        .numLight = 0,
    };
}

void FLight::AddLight(float Position[4], float Color[4], float Intensity, float MaxDistance)
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

    LightBufferData.intensityDistance[CurIndex] = { Intensity, MaxDistance, 0, 0 };

    LightBufferData.numLight++;
}

void UpdateDirectionallightViewProjectionMatrix(FScene* Scene, XMMATRIX ViewProjectionMatrix[])
{
    int DIndex = 0;
    if (GNumCascadeShadowMap == 1)
    {
        ViewProjectionMatrix[0] = Scene->GetCamera().GetDirectionalShadowViewProjMatrix(
            Scene->Light.LightBufferData.lightPosition[DIndex],
            Scene->Light.LightBufferData.intensityDistance[DIndex].y,
            0
        );
    }
    else
    {
        for (int CascadeIndex = 0; CascadeIndex < GNumCascadeShadowMap; CascadeIndex++)
        {
            ViewProjectionMatrix[CascadeIndex] = Scene->GetCamera().GetDirectionalShadowViewProjMatrix(
                Scene->Light.LightBufferData.lightPosition[DIndex],
                Scene->Light.LightBufferData.intensityDistance[DIndex].y,
                CascadeIndex
            );
        }
    }
}

interlop::LightBuffer FLight::GetLightBufferWithViewUpdate(FScene* Scene, XMMATRIX ViewMatrix)
{
    for (int i = 0; i < LightBufferData.numLight; i++)
    {
        XMFLOAT4 LightPositionXM = LightBufferData.lightPosition[i];

        
        if (i == 0)
        {
            XMVECTOR Vec = XMVector4Transform(Dx::XMLoadFloat4(&LightPositionXM), ViewMatrix);
            Dx::XMStoreFloat4(&LightBufferData.viewSpaceLightPosition[i], XMVector3Normalize(Vec));
        }
        else
        {
            XMVECTOR Pos = XMVector4Transform(Dx::XMLoadFloat4(&LightPositionXM), ViewMatrix);
            Dx::XMStoreFloat4(&LightBufferData.viewSpaceLightPosition[i], Pos);
        }
    }

    return LightBufferData;
}

interlop::ShadowBuffer FLight::GetShadowBuffer(FScene* Scene)
{
    for (int i = 0; i < LightBufferData.numLight; i++)
    {
        if (i == 0)
        {
            UpdateDirectionallightViewProjectionMatrix(Scene, ShadowBufferData.lightViewProjectionMatrix);
        }
    }

    return ShadowBufferData;
}

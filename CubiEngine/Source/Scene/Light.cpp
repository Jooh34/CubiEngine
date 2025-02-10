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

    LightBufferData.intensity[CurIndex] = Intensity;
    LightBufferData.maxDistance[CurIndex] = MaxDistance;

    LightBufferData.numLight++;
}

void UpdateDirectionallightViewProjectionMatrix(FScene* Scene, XMMATRIX ViewProjectionMatrix[])
{
    int DIndex = 0;
    if (GNumCascadeShadowMap == 1)
    {
        ViewProjectionMatrix[0] = Scene->GetCamera().GetDirectionalShadowViewProjMatrix(
            Scene->Light.LightBufferData.lightPosition[DIndex],
            Scene->Light.LightBufferData.maxDistance[DIndex],
            0
        );
    }
    else
    {
        for (int CascadeIndex = 0; CascadeIndex < GNumCascadeShadowMap; CascadeIndex++)
        {
            ViewProjectionMatrix[CascadeIndex] = Scene->GetCamera().GetDirectionalShadowViewProjMatrix(
                Scene->Light.LightBufferData.lightPosition[DIndex],
                Scene->Light.LightBufferData.maxDistance[DIndex],
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

        XMVECTOR Vec = XMVector4Transform(Dx::XMLoadFloat4(&LightPositionXM), ViewMatrix);
        Dx::XMStoreFloat4(&LightBufferData.viewSpaceLightPosition[i], XMVector3Normalize(Vec));
        
        if (i == 0)
        {
            UpdateDirectionallightViewProjectionMatrix(Scene, LightBufferData.lightViewProjectionMatrix);
        }
    }

    return LightBufferData;
}

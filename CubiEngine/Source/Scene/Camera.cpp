#include "Scene/Camera.h"

FCamera::FCamera(uint32_t Width, uint32_t Height)
{
    CamPosition = { 0.f,5.f,-30.f,1.f };
    MovementSpeed = 0.1f;
    RotationSpeed = 0.005f;
    Pitch = 0.f;
    Yaw = 0.f;
    Roll = 0.f;

    FovY = Dx::XM_PIDIV4;
    AspectRatio = static_cast<float>(Width) / Height;
    NearZ = 1.f;
    FarZ = 3000.f;

    UpdateMatrix();
}

void FCamera::Update(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height)
{
    CamPositionXMV = XMLoadFloat4(&CamPosition);

    AspectRatio = static_cast<float>(Width) / Height;

    XMVECTOR MoveVector = XMVECTOR{ 0.f, 0.f, 0.f, 0.f };

    float PitchVector = Input->GetDY() * RotationSpeed;
    float YawVector = Input->GetDX() * RotationSpeed;
    
    float Boost = 1.f;
    if (Input->GetKeyState(SDL_SCANCODE_LSHIFT))
    {
        Boost = 6.f;
    }
    else if (Input->GetKeyState(SDL_SCANCODE_LCTRL))
    {
        Boost = 0.2f;
    }

    if (Input->GetKeyState(SDL_SCANCODE_W))
    {
        MoveVector = XMVectorAdd(MoveVector, CamFront);
    }
    if (Input->GetKeyState(SDL_SCANCODE_S))
    {
        MoveVector = XMVectorSubtract(MoveVector, CamFront);
    }
    if (Input->GetKeyState(SDL_SCANCODE_D))
    {
        MoveVector = XMVectorAdd(MoveVector, CamRight);
    }
    if (Input->GetKeyState(SDL_SCANCODE_A))
    {
        MoveVector = XMVectorSubtract(MoveVector, CamRight);
    }
    if (Input->GetKeyState(SDL_SCANCODE_Q))
    {
        MoveVector = XMVectorAdd(MoveVector, WorldUp);
    }
    if (Input->GetKeyState(SDL_SCANCODE_E))
    {
        MoveVector = XMVectorSubtract(MoveVector,WorldUp);
    }

    float Speed = MovementSpeed * DeltaTime * Boost;
    MoveVector = XMVectorScale(XMVector3Normalize(MoveVector), Speed);
    CamPositionXMV = XMVectorAdd(CamPositionXMV, MoveVector);
    
    Pitch = std::clamp(Pitch + PitchVector, (float)-M_PI * 89/180.f, (float)M_PI * 89/180.f);
    Yaw += YawVector;

    XMStoreFloat4(&CamPosition, CamPositionXMV);

    UpdateMatrix();
}

void FCamera::UpdateMatrix()
{
    const XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

    static constexpr XMVECTOR WorldUpVector = XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f };
    
    CamFront = XMVector3Normalize(XMVector3TransformCoord(WorldFront, RotationMatrix));
    CamRight = XMVector3Normalize(XMVector3TransformCoord(WorldRight, RotationMatrix));
    CamUp = XMVector3Normalize(XMVector3TransformCoord(WorldUp, RotationMatrix));
    const XMVECTOR CamTarget = XMVectorAdd(CamPositionXMV, CamFront);

    ViewMatrix = XMMatrixLookAtLH(CamPositionXMV, CamTarget, WorldUpVector);
    ProjMatrix = XMMatrixPerspectiveFovLH(FovY, AspectRatio, NearZ, FarZ);

    // https://iolite-engine.com/blog_posts/reverse_z_cheatsheet
    XMMATRIX M_I = {
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,  0.0f,
        0.0f,  0.0f, -1.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  1.0f
    };
    ProjMatrix = XMMatrixMultiply(ProjMatrix, M_I); // ReversedZ
}

XMMATRIX FCamera::CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Focus, XMVECTOR FrustumCorners[], float MaxZ)
{
    float LightNearZ = 1.f;

    XMVECTOR EyePos = XMVectorAdd(
        Focus,
        XMVectorScale(LightDirection, -100.f)
    );
    XMVECTOR UpVector = Dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (Dx::XMVector3Equal(Dx::XMVector3Cross(UpVector, LightDirection), Dx::XMVectorZero())) {
        UpVector = Dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }
    
    XMMATRIX ViewMatrix = XMMatrixLookAtLH(EyePos, Focus, UpVector);

    XMVECTOR LightSpaceMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
    XMVECTOR LightSpaceMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);
    for (int j = 0; j < 8; j++)
    {
        FrustumCorners[j] = XMVector4Transform(FrustumCorners[j], ViewMatrix);
        LightSpaceMin = XMVectorMin(LightSpaceMin, FrustumCorners[j]);
        LightSpaceMax = XMVectorMax(LightSpaceMax, FrustumCorners[j]);
    }
    //XMMATRIX OrthoMatrix = Dx::XMMatrixOrthographicOffCenterLH(
    //     Dx::XMVectorGetX(MinPoint), Dx::XMVectorGetX(MaxPoint),
    //     Dx::XMVectorGetY(MinPoint), Dx::XMVectorGetY(MaxPoint),
    //     Dx::XMVectorGetZ(MinPoint), Dx::XMVectorGetZ(MaxPoint)
    //);
    XMMATRIX OrthoMatrix = Dx::XMMatrixOrthographicLH(
        Dx::XMVectorGetX(LightSpaceMax) - Dx::XMVectorGetX(LightSpaceMin),
        Dx::XMVectorGetY(LightSpaceMax) - Dx::XMVectorGetY(LightSpaceMin),
        Dx::XMVectorGetZ(LightSpaceMin) - 3*(Dx::XMVectorGetZ(LightSpaceMax)-Dx::XMVectorGetZ(LightSpaceMin)),
        Dx::XMVectorGetZ(LightSpaceMax)
    );

    // https://iolite-engine.com/blog_posts/reverse_z_cheatsheet
    XMMATRIX M_I = {
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,  0.0f,
        0.0f,  0.0f, -1.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  1.0f
    };
    OrthoMatrix = XMMatrixMultiply(OrthoMatrix, M_I); // ReversedZ

    return XMMatrixMultiply(ViewMatrix, OrthoMatrix);
}

void FCamera::GetShadowBoundingBox(XMFLOAT3& Center, XMVECTOR FrustumCorners[], int CascadeIndex, XMMATRIX InverseViewProj)
{
    float FarRatio = float(CascadeIndex + 1) / GNumCascadeShadowMap;
    float NearRatio = float(CascadeIndex) / GNumCascadeShadowMap;

    for (int j = 0; j < 4; j++)
    {
        XMVECTOR direction = XMVectorSubtract(FrustumCorners[j + 4], FrustumCorners[j]);
        FrustumCorners[j] = XMVectorAdd(FrustumCorners[j], XMVectorScale(direction, NearRatio));
        FrustumCorners[j + 4] = XMVectorAdd(FrustumCorners[j], XMVectorScale(direction, FarRatio));
    }

    XMVECTOR FrustumCenter = XMVectorSet(0, 0, 0, 0);
    for (int j = 0; j < 8; j++)
    {
        FrustumCorners[j] = XMVector3TransformCoord(FrustumCorners[j], InverseViewProj);
        FrustumCenter = XMVectorAdd(FrustumCenter, FrustumCorners[j]);
    }
    FrustumCenter = XMVectorDivide(FrustumCenter, XMVectorSet(8, 8, 8, 1));

    XMStoreFloat3(&Center, FrustumCenter);
}

XMMATRIX FCamera::GetDirectionalShadowViewProjMatrix(const XMFLOAT4& LightDirection, float MaxDistance, int CascadeIndex)
{
    XMFLOAT3 CameraVFCenter;
    XMVECTOR FrustumCorners[8] = {
        XMVectorSet(-1,  1, 1, 1), XMVectorSet(1,  1, 1, 1),
        XMVectorSet(1, -1, 1, 1), XMVectorSet(-1, -1, 1, 1),
        XMVectorSet(-1,  1, 0, 1), XMVectorSet(1,  1, 0, 1),
        XMVectorSet(1, -1, 0, 1), XMVectorSet(-1, -1, 0, 1),
    }; // reversedZ

    XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, GetViewProjMatrix());
    GetShadowBoundingBox(CameraVFCenter, FrustumCorners, CascadeIndex, InvViewProjMatrix);
    
    XMMATRIX LightViewProjectionMatrix = CalculateLightViewProjMatrix(XMLoadFloat4(&LightDirection), XMLoadFloat3(&CameraVFCenter), FrustumCorners, MaxDistance);
    return LightViewProjectionMatrix;
}

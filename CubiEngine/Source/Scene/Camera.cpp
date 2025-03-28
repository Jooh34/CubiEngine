#include "Scene/Camera.h"
#include <cmath>

static const int MAX_HALTON_SEQUENCE = 16;
static const XMFLOAT2 HALTON_SEQUENCE[MAX_HALTON_SEQUENCE] = {
    XMFLOAT2(0.5f, 0.333333f),
    XMFLOAT2(0.25f, 0.666667f),
    XMFLOAT2(0.75f, 0.111111f),
    XMFLOAT2(0.125f, 0.444444f),
    XMFLOAT2(0.625f, 0.777778f),
    XMFLOAT2(0.375f, 0.222222f),
    XMFLOAT2(0.875f, 0.555556f),
    XMFLOAT2(0.0625f, 0.888889f),
    XMFLOAT2(0.5625f, 0.037037f),
    XMFLOAT2(0.3125f, 0.37037f),
    XMFLOAT2(0.8125f, 0.703704f),
    XMFLOAT2(0.1875f, 0.148148f),
    XMFLOAT2(0.6875f, 0.481482f),
    XMFLOAT2(0.4375f, 0.814815f),
    XMFLOAT2(0.9375f, 0.259259f),
    XMFLOAT2(0.03125f, 0.592593f)
};
XMFLOAT2 GetHaltonJitterOffset(uint32_t FrameIndex, float ScreenWidth, float ScreenHeight)
{
    XMFLOAT2 jitter = HALTON_SEQUENCE[FrameIndex % MAX_HALTON_SEQUENCE];

    jitter.x -= 0.5f;
    jitter.y -= 0.5f;

    return XMFLOAT2(jitter.x / (ScreenWidth), jitter.y / (ScreenHeight));
}

FCamera::FCamera(uint32_t Width, uint32_t Height)
{
    CamPosition = { 0.f,5.f,-30.f,1.f };
    MovementSpeed = 0.1f;
    RotationSpeed = 0.005f;
    Pitch = 0.f;
    Yaw = 0.f;
    Roll = 0.f;

    FovY = 45.f;
    AspectRatio = static_cast<float>(Width) / Height;
    NearZ = 1.f;
    FarZ = 4000.f;
    
    this->Width = Width;
    this->Height = Height;

    UpdateMatrix(false);
}

void FCamera::Update(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height, bool bApplyTAAJitter, float CSMExponentialFactor)
{
    CamPositionXMV = XMLoadFloat4(&CamPosition);

    AspectRatio = static_cast<float>(Width) / Height;
    this->Width = Width;
    this->Height = Height;

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
    
    // Shadow
    this->CSMExponentialFactor = CSMExponentialFactor;

    UpdateMatrix(bApplyTAAJitter);
}

void FCamera::UpdateMatrix(bool bApplyTAAJitter)
{
    const XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

    static constexpr XMVECTOR WorldUpVector = XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f };
    
    CamFront = XMVector3Normalize(XMVector3TransformCoord(WorldFront, RotationMatrix));
    CamRight = XMVector3Normalize(XMVector3TransformCoord(WorldRight, RotationMatrix));
    CamUp = XMVector3Normalize(XMVector3TransformCoord(WorldUp, RotationMatrix));
    const XMVECTOR CamTarget = XMVectorAdd(CamPositionXMV, CamFront);

    PrevViewMatrix = ViewMatrix;
    PrevProjMatrix = ProjMatrix;
    ViewMatrix = XMMatrixLookAtLH(CamPositionXMV, CamTarget, WorldUpVector);
    
    float RadianFovY = FovY * (M_PI / 180.f);
    ProjMatrix = XMMatrixPerspectiveFovLH(RadianFovY, AspectRatio, NearZ, FarZ);

    // https://iolite-engine.com/blog_posts/reverse_z_cheatsheet
    XMMATRIX M_I = {
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,  0.0f,
        0.0f,  0.0f, -1.0f,  0.0f,
        0.0f,  0.0f,  1.0f,  1.0f
    };
    ProjMatrix = XMMatrixMultiply(ProjMatrix, M_I); // ReversedZ
    
    if (bApplyTAAJitter)
    {
        XMFLOAT2 JitterOffset_Current = GetHaltonJitterOffset(GFrameCount, Width, Height);

        XMMATRIX JitterMatrix = {
            1.0f,  0.0f,  0.0f, 0.0f,
            0.0f,  1.0f,  0.0f, 0.0f,
            0.0f,  0.0f,  1.0f, 0.0f,
            JitterOffset_Current.x,  JitterOffset_Current.y,  0.0f, 1.0f
        };
        ProjMatrix = XMMatrixMultiply(ProjMatrix, JitterMatrix);
    }
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

float FCamera::GetDepthRaito(int CascadeIndex, float* OutLinearRatio)
{
    if (CascadeIndex == 0)
    {
        if (OutLinearRatio) *OutLinearRatio = 0.f;
        return 1.f;
    }
    if (CascadeIndex == GNumCascadeShadowMap)
    {
        if (OutLinearRatio) *OutLinearRatio = 1.f;
        return 0.f;
    }

    float Ratio = 1.f - pow(CSMExponentialFactor, CascadeIndex);
    if (OutLinearRatio) *OutLinearRatio = Ratio;

    float Depth = FarZ - NearZ;
    XMVECTOR DepthVector = XMVector3TransformCoord(XMVectorSet(0, 0, Depth * Ratio, 1), GetProjMatrix());

    return Dx::XMVectorGetZ(DepthVector) / Dx::XMVectorGetW(DepthVector);
}

void FCamera::GetShadowBoundingBox(XMFLOAT3& Center, XMVECTOR FrustumCorners[], int CascadeIndex, XMMATRIX InverseViewProj)
{
    float FarRatio = GetDepthRaito(CascadeIndex + 1);
    float NearRatio = GetDepthRaito(CascadeIndex);

    for (int j = 0; j < 4; j++)
    {
        FrustumCorners[j] = Dx::XMVectorSetZ(FrustumCorners[j], NearRatio);
        FrustumCorners[j+4] = Dx::XMVectorSetZ(FrustumCorners[j + 4], FarRatio);
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

XMMATRIX FCamera::GetDirectionalShadowViewProjMatrix(const XMFLOAT4& LightDirection, float MaxDistance, int CascadeIndex, float& NearDistance)
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

    float NearLinearRatio;
    GetDepthRaito(CascadeIndex, &NearLinearRatio);
    float Depth = FarZ - NearZ;
    NearDistance = NearLinearRatio * Depth;
    
    XMMATRIX LightViewProjectionMatrix = CalculateLightViewProjMatrix(XMLoadFloat4(&LightDirection), XMLoadFloat3(&CameraVFCenter), FrustumCorners, MaxDistance);
    return LightViewProjectionMatrix;
}

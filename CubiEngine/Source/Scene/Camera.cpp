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
    NearZ = 0.1f;
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
        Boost = 4.f;
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
}

XMMATRIX FCamera::CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Focus, float Radius, float MaxZ)
{
    float Width = Radius * 2.0f;
    float Height = Radius * 2.0f;

    XMVECTOR EyePos = XMVectorAdd(
        Focus,
        XMVectorScale(LightDirection, -1.0f * MaxZ)
    );
    XMVECTOR UpVector = Dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (Dx::XMVector3Equal(Dx::XMVector3Cross(UpVector, LightDirection), Dx::XMVectorZero())) {
        UpVector = Dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    XMMATRIX ViewMatrix = XMMatrixLookAtLH(EyePos, Focus, UpVector);
    XMMATRIX OrthoMatrix = Dx::XMMatrixOrthographicLH(Width, Height, 0.f, (MaxZ+ Radius));
    return XMMatrixMultiply(ViewMatrix, OrthoMatrix);
}

void FCamera::GetViewFrustumCenterAndRadius(XMFLOAT3& Center, float& Radius)
{
    float ny = NearZ * tan(FovY / 2.f);
    float nx = ny * AspectRatio;
    float fy = FarZ * tan(FovY / 2.f);
    float fx = fy * AspectRatio;
    
    float HalfZ = (NearZ + FarZ) / 2.f;
    Center = XMFLOAT3(0, 0, HalfZ);
    Radius = sqrt(fx * fx + fy * fy + (FarZ - HalfZ) * (FarZ - HalfZ));
}

XMMATRIX FCamera::GetDirectionalShadowViewProjMatrix(const XMFLOAT4& LightDirection)
{
    XMFLOAT3 CameraVFCenter;
    float Radius = 1.f;
    GetViewFrustumCenterAndRadius(CameraVFCenter, Radius);
    
    XMMATRIX InvViewMaterix = XMMatrixInverse(nullptr, ViewMatrix);
    XMVECTOR CameraVFCenterWorld = XMVector3TransformCoord(XMLoadFloat3(&CameraVFCenter), InvViewMaterix);
    
    XMMATRIX LightViewProjectionMatrix = CalculateLightViewProjMatrix(XMLoadFloat4(&LightDirection), CameraVFCenterWorld, Radius, 3000.f);
    return LightViewProjectionMatrix;
}

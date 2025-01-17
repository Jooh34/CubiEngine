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
        Boost = 4.f;
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


#include "Scene/Camera.h"

FCamera::FCamera(uint32_t Width, uint32_t Height)
{
    CamPosition = { 0.f,5.f,-30.f,1.f };
    Pitch = 0.f;
    Yaw = 0.f;
    Roll = 0.f;

    FovY = Dx::XM_PIDIV4;
    AspectRatio = static_cast<float>(Width) / Height;
    NearZ = 0.1f;
    FarZ = 1000.f;

    UpdateMatrix();
}

void FCamera::UpdateMatrix()
{
    const XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

    static constexpr XMVECTOR WorldUpVector = XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f };
    static constexpr XMVECTOR WorldFront = XMVECTOR{ 0.0f, 0.0f, 1.0f, 0.0f };
    
    const XMVECTOR CamFront = XMVector3Normalize(XMVector3TransformCoord(WorldFront, RotationMatrix));
    const XMVECTOR CamTarget = Dx::XMVectorAdd(CamPosition, CamFront);

    ViewMatrix = XMMatrixLookAtLH(CamPosition, CamTarget, WorldUpVector);
    ProjMatrix = XMMatrixPerspectiveFovLH(FovY, AspectRatio, NearZ, FarZ);
}

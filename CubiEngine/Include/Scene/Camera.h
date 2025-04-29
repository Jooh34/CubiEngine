#pragma once

#include "Core/Input.h"

class FCamera
{
public:
    FCamera(uint32_t Width, uint32_t Height);
    
    void Update(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height, bool bApplyTAAJitter, float CSMExponentialFactor);
    void UpdateMatrix(bool bApplyTAAJitter);

    void SetCamPosition(float X, float Y, float Z)
    {
        CamPosition = { X, Y, Z, 1.f };
    };
    void SetCamRotation(float InPitch, float InYaw, float InRoll)
    {
        Pitch = InPitch;
        Yaw = InYaw;
        Roll = InRoll;
    };

    XMMATRIX GetViewMatrix() const { return ViewMatrix; }
    XMMATRIX GetProjMatrix() const { return ProjMatrix; }
    XMMATRIX GetViewProjMatrix() const { return ViewMatrix * ProjMatrix; }
    XMMATRIX GetPrevViewProjMatrix() const {
        return PrevViewMatrix * PrevProjMatrix;
    }

    XMMATRIX GetInvViewProjMatrix() const {
        return XMMatrixInverse(nullptr, GetViewProjMatrix());
    }

    XMFLOAT4& GetCameraPosition() { return CamPosition; }
    XMVECTOR GetCameraPositionXMV() const { return CamPositionXMV; }
    
    XMMATRIX CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Focus, XMVECTOR FrustumCorners[], float MaxZ);
    float GetDepthRaito(int CascadeIndex, float* OutLinearRatio = nullptr);
    void GetShadowBoundingBox(XMFLOAT3& Center, XMVECTOR FrustumCorners[], int CascadeIndex, XMMATRIX InverseViewProj);

    XMMATRIX GetDirectionalShadowViewProjMatrix(const XMFLOAT4& LightPosition, float MaxDistance, int CascadeIndex, float& NearDistance);
    
    // from UE5
    XMFLOAT4 CreateInvDeviceZToWorldZTransform(const XMMATRIX& ProjMatrix);
    XMMATRIX GetClipToPrevClip();

    float FarZ;
    float NearZ;
    float FovY;

private:
    uint32_t Width;
    uint32_t Height;
    float MovementSpeed{};
    float RotationSpeed{};

    XMFLOAT4 CamPosition { 0.f,0.f,0.f,1.f };
    float Pitch = 0.f;
    float Yaw = 0.f;
    float Roll = 0.f;

    // use below read only
    XMVECTOR CamPositionXMV { 0.f,0.f,0.f,1.f };
    XMVECTOR CamFront{ 0.f, 0.f, 1.f, 0.f };
    XMVECTOR CamRight{ 1.f, 0.f, 0.f, 0.f };
    XMVECTOR CamUp{ 0.f, 1.f, 0.f, 0.f };

    XMMATRIX ViewMatrix{};
    XMMATRIX ProjMatrix{};
    XMMATRIX PrevViewMatrix{};
    XMMATRIX PrevProjMatrix{};

    // Projection Matrix
    float AspectRatio;

    static constexpr XMVECTOR WorldFront = XMVECTOR{ 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr XMVECTOR WorldRight = XMVECTOR{ 1.0f, 0.0f, 0.0f, 0.0f };
    static constexpr XMVECTOR WorldUp = XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f };

    // Shadow
    float CSMExponentialFactor = 0.8;
};
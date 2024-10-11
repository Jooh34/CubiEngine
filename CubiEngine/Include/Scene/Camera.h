#pragma once

#include "Core/Input.h"

class FCamera
{
public:
    FCamera(uint32_t Width, uint32_t Height);
    
    void Update(float DeltaTime, FInput* Input);
    void UpdateMatrix();

    XMMATRIX GetViewMatrix() const { return ViewMatrix; }
    XMMATRIX GetProjMatrix() const { return ProjMatrix; }
    XMMATRIX GetViewProjMatrix() const { return ViewMatrix * ProjMatrix; }

private:
    float MovementSpeed{};
    float RotationSpeed{};

    XMVECTOR CamPosition { 0.f,0.f,0.f,1.f };
    XMVECTOR CamFront{ 0.f, 0.f, 1.f, 0.f };
    XMVECTOR CamRight{ 1.f, 0.f, 0.f, 0.f };
    XMVECTOR CamUp{ 0.f, 1.f, 0.f, 0.f };
    float Pitch = 0.f;
    float Yaw = 0.f;
    float Roll = 0.f;

    XMMATRIX ViewMatrix{};
    XMMATRIX ProjMatrix{};

    // Projection Matrix
    float FovY;
    float AspectRatio;
    float NearZ;
    float FarZ;

    static constexpr XMVECTOR WorldFront = XMVECTOR{ 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr XMVECTOR WorldRight = XMVECTOR{ 1.0f, 0.0f, 0.0f, 0.0f };
    static constexpr XMVECTOR WorldUp = XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f };
};
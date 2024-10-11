#pragma once

class FCamera
{
public:
    FCamera(uint32_t Width, uint32_t Height);

    XMMATRIX GetViewMatrix() const { return ViewMatrix; }
    XMMATRIX GetProjMatrix() const { return ProjMatrix; }
    XMMATRIX GetViewProjMatrix() const { return ViewMatrix * ProjMatrix; }
    void UpdateMatrix();

private:
    XMVECTOR CamPosition { 0.f,0.f,0.f,1.f };
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
};
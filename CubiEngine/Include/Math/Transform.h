#pragma once

struct FTransform
{
    XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    XMMATRIX TransformMatrix{ Dx::XMMatrixIdentity() };
	XMMATRIX InverseTransformMatrix{ Dx::XMMatrixIdentity() };

    void Set(const XMFLOAT3& InRotation, const XMFLOAT3& InScale, const XMFLOAT3& InTranslate);

	XMMATRIX GetModelMatrix() const { return TransformMatrix; }
	XMMATRIX GetInverseModelMatrix() const { return InverseTransformMatrix; }
};
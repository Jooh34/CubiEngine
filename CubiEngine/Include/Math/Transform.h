#pragma once

class FTransform
{
public:
    void Set(const XMFLOAT3& InRotation, const XMFLOAT3& InScale, const XMFLOAT3& InTranslate);
	FTransform Multiply(const FTransform& Other) const;

	XMMATRIX GetModelMatrix() const { return TransformMatrix; }
	XMMATRIX GetInverseModelMatrix() const { return InverseTransformMatrix; }

private:
    XMMATRIX TransformMatrix{ Dx::XMMatrixIdentity() };
	XMMATRIX InverseTransformMatrix{ Dx::XMMatrixIdentity() };
};
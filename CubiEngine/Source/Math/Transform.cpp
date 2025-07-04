#include "Math/Transform.h"

void FTransform::Set(const XMFLOAT3& InRotation, const XMFLOAT3& InScale, const XMFLOAT3& InTranslate)
{
    const XMVECTOR scalingVector = XMLoadFloat3(&InScale);
    const XMVECTOR rotationVector = XMLoadFloat3(&InRotation);
    const XMVECTOR translationVector = XMLoadFloat3(&InTranslate);

    const XMMATRIX modelMatrix = Dx::XMMatrixScalingFromVector(scalingVector) *
        Dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) *
        Dx::XMMatrixTranslationFromVector(translationVector);

    TransformMatrix = modelMatrix;
	InverseTransformMatrix = Dx::XMMatrixInverse(nullptr, modelMatrix);
}

FTransform FTransform::Multiply(const FTransform& Other) const
{
    FTransform Transform{};
    Transform.TransformMatrix = Dx::XMMatrixMultiply(TransformMatrix, Other.TransformMatrix);
    Transform.InverseTransformMatrix = Dx::XMMatrixInverse(nullptr, Transform.TransformMatrix);
    return Transform;
}

#include "Math/Transform.h"

void FTransform::Set(const XMFLOAT3& InRotation, const XMFLOAT3& InScale, const XMFLOAT3& InTranslate)
{
    Rotation = InRotation;
    Scale = InScale;
    Translate = InTranslate;

    const XMVECTOR scalingVector = XMLoadFloat3(&Scale);
    const XMVECTOR rotationVector = XMLoadFloat3(&Rotation);
    const XMVECTOR translationVector = XMLoadFloat3(&Translate);

    const XMMATRIX modelMatrix = Dx::XMMatrixScalingFromVector(scalingVector) *
        Dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) *
        Dx::XMMatrixTranslationFromVector(translationVector);

    TransformMatrix = modelMatrix;
	InverseTransformMatrix = Dx::XMMatrixInverse(nullptr, modelMatrix);
}
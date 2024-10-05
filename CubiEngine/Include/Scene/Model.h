#pragma once

class FGraphicsDevice;

struct TransformComponent
{
    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    //Buffer transformBuffer{};

    void update();

};

struct ModelCreationDesc
{
    std::string_view ModelPath{};
    std::string_view ModelName{};

    Dx::XMFLOAT3 Rotation{0.0f, 0.0f, 0.0f};
    Dx::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};
    Dx::XMFLOAT3 Translate{0.0f, 0.0f, 0.0f};
};

class FModel
{
public:
    FModel(const FGraphicsDevice* const GraphicsDevice, const ModelCreationDesc& ModelCreationDesc);

private:
    std::string Name;
};
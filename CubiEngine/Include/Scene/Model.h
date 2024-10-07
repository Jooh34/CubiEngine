#pragma once

#include "Graphics/Resource.h"

class FGraphicsDevice;

struct FTransform
{
    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    //Buffer transformBuffer{};

    void update();

};

struct FModelCreationDesc
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
    FModel(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc);

    void LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);


private:
    std::string Name;
    std::string ModelDir;

    std::vector<FSampler> Samplers;
    std::vector<FPBRMaterial> Materials;
};
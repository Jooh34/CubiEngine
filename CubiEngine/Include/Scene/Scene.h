#pragma once

#include "Scene/Model.h"

class FScene
{
public:
    FScene();

    std::unordered_map<std::string, std::unique_ptr<FModel>> Models{};
    void AddModel(const FGraphicsDevice* Device, const ModelCreationDesc& Desc);
};
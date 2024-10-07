#pragma once

#include "Scene/Model.h"

class FScene
{
public:
    FScene(FGraphicsDevice* Device);
    ~FScene();

    void AddModel(const FModelCreationDesc& Desc);

private:
    std::unordered_map<std::string, std::unique_ptr<FModel>> Models{};
    FGraphicsDevice* Device;
};
#pragma once

#include "Scene/Model.h"
#include "ShaderInterlop/RenderResources.hlsli"

class FGraphicsContext;
class FScene
{
public:
    FScene(FGraphicsDevice* Device);
    ~FScene();

    void AddModel(const FModelCreationDesc& Desc);

    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::UnlitPassRenderResources& UnlitRenderResources);

private:
    std::unordered_map<std::string, std::unique_ptr<FModel>> Models{};
    FGraphicsDevice* Device;
};
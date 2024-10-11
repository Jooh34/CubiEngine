#pragma once

#include "Scene/Model.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Camera.h"

class FGraphicsContext;
class FCamera;

class FScene
{
public:
    FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height);
    ~FScene();
    
    void Update();
    void AddModel(const FModelCreationDesc& Desc);

    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::UnlitPassRenderResources& UnlitRenderResources);

private:
    std::unordered_map<std::string, std::unique_ptr<FModel>> Models{};
    FGraphicsDevice* Device;

    FCamera Camera;
    FBuffer SceneBuffer{};
};
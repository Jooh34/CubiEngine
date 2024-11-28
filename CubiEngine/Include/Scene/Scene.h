#pragma once

#include "Scene/Model.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Camera.h"
#include "Scene/Light.h"

class FGraphicsContext;
class FCamera;
class FInput;

class FScene
{
public:
    FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height);
    ~FScene();
    
    void Update(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height);
    void AddModel(const FModelCreationDesc& Desc);
    void AddLight(float Position[4], float Color[4]) { Light.AddLight(Position, Color); }

    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::UnlitPassRenderResources& UnlitRenderResources);
    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::DeferredGPassRenderResources& DeferredGRenderResources);

    FBuffer& GetSceneBuffer() { return SceneBuffer; }
    FBuffer& GetLightBuffer() { return LightBuffer; }

private:
    std::unordered_map<std::wstring, std::unique_ptr<FModel>> Models{};
    FGraphicsDevice* Device;

    FCamera Camera;
    FLight Light;
    FBuffer SceneBuffer{};
    FBuffer LightBuffer{};
};
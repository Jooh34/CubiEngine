#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"

FScene::FScene(FGraphicsDevice* Device) : Device(Device)
{
    FModelCreationDesc Desc{
        .ModelPath = "Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
        .ModelName = "MetalRoughSpheres",
    };

    AddModel(Desc);
}

FScene::~FScene()
{
}

void FScene::AddModel(const FModelCreationDesc& Desc)
{
    const std::string Key{ Desc.ModelName };

    Models[Key] = std::make_unique<FModel>(Device, Desc);
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources)
{
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, UnlitRenderResources);
    }
}

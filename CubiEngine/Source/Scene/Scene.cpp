#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"

FScene::FScene()
{
    
}

void FScene::AddModel(const FGraphicsDevice* Device, const ModelCreationDesc& Desc)
{
    const std::string Key{ Desc.ModelName };

    Models[Key] = std::make_unique<FModel>(Device, Desc);
}

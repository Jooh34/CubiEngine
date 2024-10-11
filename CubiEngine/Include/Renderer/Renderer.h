#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "Renderer/UnlitPass.h"

class FInput;
class FRenderer
{
public:
    FRenderer(FGraphicsDevice* GraphicsDevice, uint32_t Width, uint32_t Height);
    ~FRenderer();

    void Update(float DeltaTime, FInput* Input);
    void Render();
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice; }

private:
    FGraphicsDevice* GraphicsDevice;

    uint32_t Width{};
    uint32_t Height{};
    FTexture DepthTexture;

    std::unique_ptr<FScene> Scene;
    std::unique_ptr<FUnlitPass> UnlitPass;
};
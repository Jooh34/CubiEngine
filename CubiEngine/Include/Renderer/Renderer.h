#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "Renderer/UnlitPass.h"

class FRenderer
{
public:
    FRenderer(HWND Handle, uint32_t Width, uint32_t Height);
    ~FRenderer();

    void Render();
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice.get(); }

private:
    std::unique_ptr<FUnlitPass> UnlitPass;
    FTexture DepthTexture;

    std::unique_ptr<FGraphicsDevice> GraphicsDevice;
    std::unique_ptr<FScene> Scene;

    float RenderTargetColorTest[4] = { 0,0,0,1 };

};
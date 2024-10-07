#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"

class FRenderer
{
public:
    FRenderer(HWND Handle, int Width, int Height);
    ~FRenderer();

    void Render();
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice.get(); }

private:
    std::unique_ptr<FGraphicsDevice> GraphicsDevice;
    std::unique_ptr<FScene> Scene;

    float RenderTargetColorTest[4] = { 0,0,0,1 };

};
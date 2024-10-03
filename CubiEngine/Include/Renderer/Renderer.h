#pragma once

#include "Graphics/GraphicsDevice.h"

class FRenderer
{
public:
    FRenderer(HWND Handle, int Width, int Height);
    ~FRenderer();

    void Render();
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice.get(); }

private:
    std::unique_ptr<FGraphicsDevice> GraphicsDevice;

    float RenderTargetColorTest[4] = { 0,0,0,1 };
};
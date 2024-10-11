#pragma once

#include <string>
#include "Core/Input.h"
#include "Graphics/GraphicsDevice.h"

class FRenderer;

class Application
{
public:
    Application(const std::string& Title);
    virtual ~Application();

    bool Init(uint32_t Width, uint32_t Height);

    void Run();
    void Render(float DeltaTime);
    void Cleanup();

    void HandleEvents();

private:
    SDL_Window* Window;
    HWND WindowHandle{};
    std::string WindowTitle;

    bool IsRunning;
    
    // Device Should be here because all elements of FRenderer should be released before FGraphicsDevice releases.
    std::unique_ptr<FGraphicsDevice> GraphicsDevice;
    FRenderer* D3DRenderer;
    FInput Input;
};
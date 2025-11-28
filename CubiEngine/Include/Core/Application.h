#pragma once

#include <string>
#include "Core/Input.h"

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
    
    FRenderer* D3DRenderer;
    FInput Input;

    std::chrono::high_resolution_clock::time_point PrevTime;
};
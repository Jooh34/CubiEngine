#pragma once

#include <string>

class FRenderer;

class Application
{
public:
    Application(const std::string& Title);
    virtual ~Application();

    bool Init(uint32_t Width, uint32_t Height);

    void Run();
    void Render();
    void Cleanup();

    void HandleEvents();

private:
    SDL_Window* Window;
    HWND WindowHandle{};
    std::string WindowTitle;

    bool IsRunning;

    FRenderer* D3DRenderer;
};
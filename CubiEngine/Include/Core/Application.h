#pragma once

#include <string>

class FRenderer;

class Application
{
public:
    Application(const std::string& Title);
    virtual ~Application();

    bool Init(int Width, int Height);

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
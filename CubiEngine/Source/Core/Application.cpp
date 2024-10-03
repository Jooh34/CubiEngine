#include "Core/Application.h"
#include "Core/FileSystem.h"
#include "Renderer/Renderer.h"

Application::Application(const std::string& Title)
    : WindowTitle(Title)
{
}

Application::~Application()
{
}

bool Application::Init(int Width, int Height)
{
    FileSystem::LocateRootDirectory();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the window
    Window = SDL_CreateWindow(WindowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_SHOWN);
    if (!Window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Get native window handle
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(Window, &wmInfo);
    WindowHandle = wmInfo.info.win.window;

    // Initialize renderer
    D3DRenderer = new FRenderer(WindowHandle, Width, Height);

    IsRunning = true;
    return true;
}

void Application::Run()
{
    Sleep(1000);
    while (IsRunning)
    {
        HandleEvents();
        Render();
    }

    Cleanup();
}

void Application::Render()
{
    D3DRenderer->Render();
}

void Application::Cleanup()
{
    if (D3DRenderer) {
        delete D3DRenderer;
    }

    SDL_DestroyWindow(Window);
    SDL_Quit();
}

void Application::HandleEvents()
{
    SDL_Event Event;
    while (SDL_PollEvent(&Event))
    {
        if (Event.type == SDL_QUIT)
        {
            IsRunning = false;
        }
        if (Event.type == SDL_KEYDOWN)
        {
            if (Event.key.keysym.sym == SDLK_ESCAPE)
            {
                IsRunning = false;
            }
        }
    }
}

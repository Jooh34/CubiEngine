#include "Core/Application.h"
#include "Core/FileSystem.h"
#include "Renderer/Renderer.h"

#include <imgui_impl_sdl2.h>

// Setting the Agility SDK parameters.
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 715u;
}
extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

Application::Application(const std::string& Title)
    : WindowTitle(Title)
{
    std::chrono::high_resolution_clock Clock{};
    PrevTime = Clock.now();
}

Application::~Application()
{
}

bool Application::Init(uint32_t Width, uint32_t Height)
{
    FFileSystem::LocateRootDirectory();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the window
    Window = SDL_CreateWindow(WindowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_RESIZABLE);
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
    GraphicsDevice = std::make_unique<FGraphicsDevice>(
        Width, Height, DXGI_FORMAT_R10G10B10A2_UNORM, WindowHandle);
    D3DRenderer = new FRenderer(GraphicsDevice.get(), Window, Width, Height);

    IsRunning = true;
    return true;
}

void Application::Run()
{
    std::chrono::high_resolution_clock Clock{};

    while (IsRunning)
    {
        HandleEvents();

        const std::chrono::high_resolution_clock::time_point CurTime = Clock.now();
        const float DeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(CurTime - PrevTime).count();
        PrevTime = CurTime;

        Render(DeltaTime);
    }

    Cleanup();
}

void Application::Render(float DeltaTime)
{
    D3DRenderer->GameTick(DeltaTime, &Input);
    D3DRenderer->Render();
}

void Application::Cleanup()
{
    SDL_DestroyWindow(Window);
    SDL_Quit();

    // flush before clearing D3DRenderer
    if (GraphicsDevice)
    {
        GraphicsDevice->FlushAllQueue();
    }

    if (D3DRenderer) {
        delete D3DRenderer;
    }
}

void Application::HandleEvents()
{
    SDL_Event Event;
    Input.Reset();
    while (SDL_PollEvent(&Event))
    {
        ImGui_ImplSDL2_ProcessEvent(&Event);

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
        if (Event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            uint32_t Width = Event.window.data1;
            uint32_t Height = Event.window.data2;
            // TODO : Window Resize

            D3DRenderer->OnWindowResized(Width, Height);
        }
        Input.ProcessEvent(Event);
    }
}

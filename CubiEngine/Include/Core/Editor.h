#pragma once

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FEditor
{
public:
    FEditor(FGraphicsDevice* Device, SDL_Window* Window, uint32_t Width, uint32_t Height);
    ~FEditor();

    void Render(FGraphicsContext* GraphicsContext, FScene* Scene);

    void RenderDebugProperties(FScene* Scene);
    void RenderCameraProperties(FScene* Scene);
    void RenderGIProperties(FScene* Scene);
    void RenderLightProperties(FScene* Scene);
    void RenderGPUProfileData();
    void RenderProfileProperties(FScene* Scene);

    void OnWindowResized(uint32_t Width, uint32_t Height);

private:
    FGraphicsDevice* Device;
    SDL_Window* Window;
};
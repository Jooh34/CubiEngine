#pragma once

class FGraphicsDevice;
class FGraphicsContext;
class FScene;
class FInput;

class FEditor
{
public:
    FEditor(FGraphicsDevice* Device, SDL_Window* Window, uint32_t Width, uint32_t Height);
    ~FEditor();

	void GameTick(float DeltaTime, FInput* Input);
    void Render(FGraphicsContext* GraphicsContext, FScene* Scene);

    void RenderDebugProperties(FScene* Scene);
    void RenderCameraProperties(FScene* Scene);
    void RenderGIProperties(FScene* Scene);
    void RenderLightProperties(FScene* Scene);
    void RenderPostProcessProperties(FScene* Scene);
    void RenderGPUProfileData();
    void RenderProfileProperties(FScene* Scene);

    void OnWindowResized(uint32_t Width, uint32_t Height);

private:
    FGraphicsDevice* Device;
    SDL_Window* Window;

    bool bShowUI = true;
};
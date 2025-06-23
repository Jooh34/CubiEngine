#pragma once

class FGraphicsDevice;
class FScene;

class FRenderPass
{
public:
    FRenderPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);
    virtual ~FRenderPass() = default;

    void Initialize();
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    virtual void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) = 0;

protected:
    const FGraphicsDevice* GraphicsDevice;

    uint32_t Width;
    uint32_t Height;
};
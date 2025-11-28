#pragma once

class FScene;

class FRenderPass
{
public:
    FRenderPass(uint32_t Width, uint32_t Height);
    virtual ~FRenderPass() = default;

    void Initialize();
    void OnWindowResized(uint32_t InWidth, uint32_t InHeight);
    virtual void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) = 0;

protected:
    uint32_t Width;
    uint32_t Height;
};
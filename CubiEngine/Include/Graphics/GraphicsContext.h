#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"

class FGraphicsDevice;

class FGraphicsContext : public FContext
{
public:
    FGraphicsContext(FGraphicsDevice* const Device);
    void SetDescriptorHeaps() const;
    void Reset() const;

    void ClearRenderTargetView(const Texture& InRenderTarget, std::span<const float, 4> Color);

private:
    FGraphicsDevice* Device;
};
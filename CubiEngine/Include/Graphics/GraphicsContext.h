#pragma once

#include "Graphics/Context.h"

class FGraphicsDevice;

class FGraphicsContext : public FContext
{
public:
    FGraphicsContext(FGraphicsDevice* const Device);
    void SetDescriptorHeaps() const;
    void Reset() const;
private:
    FGraphicsDevice* Device;
};
#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"

class FGraphicsDevice;

class FCopyContext : public FContext
{
public:
    FCopyContext(FGraphicsDevice* const Device);
private:
};
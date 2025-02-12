#pragma once

#include "Graphics/Resource.h"
#include "WinPixEventRuntime/pix3.h"

class FEvent
{
    std::string Name;
    float Duration;

    void BeginEvent();
};

class FProfiler
{
};

#define SCOPED_NAMED_EVENT(GraphicsContext, NAME)\
    PIXScopedEventObject Event_##NAME = PIXScopedEventObject(GraphicsContext->GetCommandList(), PIX_COLOR(255, 255, 255), #NAME);

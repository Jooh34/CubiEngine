#include "Core/Input.h"

FInput::FInput()
    :MouseButtonState(0), X(0), Y(0), DX(0), DY(0)
{
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    {
        KeyStates[i] = false;
    }
}

void FInput::Reset()
{
    DX = 0;
    DY = 0;
    ResetKeyDown();
}

void FInput::ProcessEvent(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_KEYDOWN:
            KeyStates[event.key.keysym.scancode] = true;
			KeyDown[event.key.keysym.scancode] = true;
            break;
        case SDL_KEYUP:
            KeyStates[event.key.keysym.scancode] = false;
            break;
        case SDL_MOUSEMOTION:
            if (IsMouseRPressing())
            {
                PrevX = X;
                PrevY = Y;
                X = event.motion.x;
                Y = event.motion.y;
                DX = X - PrevX;
                DY = Y - PrevY;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            MouseButtonState |= SDL_BUTTON(event.button.button);
            if (IsMouseRPressing())
            {
                X = event.motion.x;
                Y = event.motion.y;
                PrevX = event.motion.x;
                PrevY = event.motion.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            MouseButtonState &= ~SDL_BUTTON(event.button.button);
            break;
    }
}

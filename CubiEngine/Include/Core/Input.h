#pragma once

class FInput
{
public:
    FInput();

    void Reset();
    void ProcessEvent(const SDL_Event& event);
    bool IsMouseRPressing() const { return !!(MouseButtonState & SDL_BUTTON_RMASK); }

    bool GetKeyState(SDL_Scancode Key) { return KeyStates[Key]; }
    bool GetKeyDown(SDL_Scancode Key) { return KeyDown[Key]; }
    float GetDX() const { return DX; }
    float GetDY() const { return DY; }

    void ResetKeyDown()
    {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            KeyDown[i] = false;
        }
    }

private:
    std::array<bool, SDL_NUM_SCANCODES> KeyStates{}; // Key states (pressed or not)
	std::array<bool, SDL_NUM_SCANCODES> KeyDown{}; // Key Down at this frame
    int X, Y;
    int DX, DY;
    Uint32 MouseButtonState;                          // Mouse button states

    int PrevX, PrevY;
};
#pragma once

class FInput
{
public:
    FInput();

    void Reset();
    void ProcessEvent(const SDL_Event& event);
    bool IsMouseRPressing() const { return !!(MouseButtonState & SDL_BUTTON_RMASK); }

    bool GetKeyState(SDL_Scancode Key) { return KeyStates[Key]; }
    float GetDX() const { return DX; }
    float GetDY() const { return DY; }

private:
    std::array<bool, SDL_NUM_SCANCODES> KeyStates{}; // Key states (pressed or not)
    int X, Y;
    int DX, DY;
    Uint32 MouseButtonState;                          // Mouse button states

    int PrevX, PrevY;
};
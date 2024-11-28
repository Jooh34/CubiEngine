#include "Core/Application.h"

int main(int argc, char* argv[])
{
    Application App("CubiEngine");

    if (!App.Init(InitialWidth, InitialHeight)) {
        return -1;
    }

    App.Run();

    return 0;
}
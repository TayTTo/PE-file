#include <windows.h>
#include <thread>  
#include <chrono>  

void ShowMessage()
{
    MessageBox(NULL, "You got fucked", "Alert", MB_OK | MB_ICONEXCLAMATION);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // Show the popup immediately
    ShowMessage();

    while (true)
    {
		ShowMessage();
        std::this_thread::sleep_for(std::chrono::seconds(5));

    }

    return 0;
}


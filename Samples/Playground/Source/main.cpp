#include "Playground.hpp"

extern "C" {
__declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
}
extern "C" {
__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

int main() {
    Playground app{{.width = 1280, .height = 720, .title = L"Playground"}};
    return app.Run();
}

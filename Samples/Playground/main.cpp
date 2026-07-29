#include "Playground.hpp"

int main() {
    Playground app{{.width = 1280, .height = 720, .title = L"Playground"}};
    return app.Run();
}

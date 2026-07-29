#pragma once

namespace GEngine {

struct Input {
    // Keyboard

    [[nodiscard]] static bool IsKeyDown(int vkCode);

    // Mouse

    [[nodiscard]] static POINT GetCursorPosition();
    static void SetCursorPosition(int x, int y);
    static void ShowCursor(bool show);
    static void HideCursor();
};

} // namespace GEngine

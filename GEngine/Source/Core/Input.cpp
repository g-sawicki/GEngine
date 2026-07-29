#include "PCH.hpp"

#include "Input.hpp"

namespace GEngine {

bool Input::IsKeyDown(int vkCode) {
    return (::GetAsyncKeyState(vkCode) & 0x8000) != 0;
}

POINT Input::GetCursorPosition() {
    POINT pos;
    ::GetCursorPos(&pos);
    return pos;
}

void Input::SetCursorPosition(int x, int y) {
    ::SetCursorPos(x, y);
}

void Input::ShowCursor(bool show) {
    ::ShowCursor(show ? TRUE : FALSE);
}

void Input::HideCursor() {
    ShowCursor(false);
}

} // namespace GEngine

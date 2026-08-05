#pragma once

#include "Core/World.hpp"
#include "Graphics/Renderer.hpp"
#include "Platform/Windows/Window.hpp"

namespace GEngine {

class Application {
  public:
    struct Specification {
        uint32_t width{};
        uint32_t height{};
        std::wstring title{};
    };

    Application(const Specification& specification);
    virtual ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int Run();

    [[nodiscard]] World& GetWorld() noexcept { return m_World; }
    [[nodiscard]] const World& GetWorld() const noexcept { return m_World; }
    [[nodiscard]] Renderer& GetRenderer() noexcept { return m_Renderer; }

  protected:
    virtual void OnInit();
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnRender();
    virtual void OnResize(uint32_t width, uint32_t height);

  private:
    void OnDestroy();
    void Render();
    void HandleResize(uint32_t width, uint32_t height);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    std::unique_ptr<Window> m_Window;
    Renderer m_Renderer;
    World m_World;

    Specification m_Specification;
    bool m_UseWarp{};

    // Timing
    uint64_t m_FrameCounter{0};
    double m_ElapsedSeconds{0.0};
    std::chrono::high_resolution_clock::time_point m_PrevTime{std::chrono::high_resolution_clock::now()};
};

} // namespace GEngine

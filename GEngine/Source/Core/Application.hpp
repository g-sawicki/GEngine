#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/Renderer.hpp"
#include "Platform/Windows/Window.hpp"
#include "Scene/Scene.hpp"

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

    GE_NO_COPY_NO_MOVE(Application)

    int Run();

  protected:
    virtual void OnInit();
    virtual void OnUpdate(float deltaTime) = 0;

  private:
    void OnDestroy();
    void Render();
    void HandleResize(uint32_t width, uint32_t height);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

  protected:
    AssetManager m_AssetManager{};
    Renderer m_Renderer{};
    Scene m_Scene{};

  private:
    std::unique_ptr<Window> m_Window;

    Specification m_Specification{};
    bool m_UseWarp{};

    // Timing
    uint64_t m_FrameCounter{};
    double m_ElapsedSeconds{};
    std::chrono::high_resolution_clock::time_point m_PrevTime{std::chrono::high_resolution_clock::now()};
};

} // namespace GEngine

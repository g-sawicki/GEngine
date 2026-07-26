add_executable(${PROJECT_NAME} WIN32)

# ── Shaders ────────────────────────────────────────────────────────────────────
include("${CMAKE_SOURCE_DIR}/cmake/ShaderCompiler.cmake")
set(SHADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders")
compile_shader("default.hlsl" "VSMain" "vs_6_5" "default_vs.h")
compile_shader("default.hlsl" "PSMain" "ps_6_5" "default_ps.h")

target_sources(${PROJECT_NAME} PRIVATE
    "Source/Graphics/D3D12/CommandList.cpp"
    "Source/Graphics/D3D12/CommandList.hpp"
    "Source/Graphics/D3D12/CommandQueue.cpp"
    "Source/Graphics/D3D12/CommandQueue.hpp"
    "Source/Graphics/D3D12/Common.hpp"
    "Source/Graphics/D3D12/Device.cpp"
    "Source/Graphics/D3D12/Device.hpp"
    "Source/Graphics/D3D12/Fence.cpp"
    "Source/Graphics/D3D12/Fence.hpp"
    "Source/Graphics/D3D12/PipelineState.cpp"
    "Source/Graphics/D3D12/PipelineState.hpp"
    "Source/Graphics/D3D12/RootSignature.cpp"
    "Source/Graphics/D3D12/RootSignature.hpp"
    "Source/Graphics/D3D12/SwapChain.cpp"
    "Source/Graphics/D3D12/SwapChain.hpp"

    "Source/Graphics/Renderer.cpp"
    "Source/Graphics/Renderer.hpp"

    "Source/main.cpp"
    "Source/PCH.hpp"
    "Source/Window.cpp"
    "Source/Window.hpp"

    "${SHADER_GEN_DIR}/default_vs.h"
    "${SHADER_GEN_DIR}/default_ps.h"
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${SHADER_GEN_DIR}")

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive- /Zc:__cplusplus /sdl /utf-8)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE "Source/PCH.hpp")

target_link_libraries(${PROJECT_NAME} PRIVATE d3d12 dxgi dxguid Microsoft::DirectX-Headers)

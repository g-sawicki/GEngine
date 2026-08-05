add_library(${PROJECT_NAME} STATIC)

# ── Shaders ────────────────────────────────────────────────────────────────────
include("${CMAKE_SOURCE_DIR}/cmake/ShaderCompiler.cmake")
set(SHADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders")
compile_shader("default.hlsl" "VSMain" "vs_6_5" "default_vs.h")
compile_shader("default.hlsl" "PSMain" "ps_6_5" "default_ps.h")

target_sources(${PROJECT_NAME} PRIVATE
    "Source/Core/Utility/Image.cpp"
    
    "Source/Core/Application.cpp"
    "Source/Core/Application.hpp"
    "Source/Core/Input.cpp"
    "Source/Core/Input.hpp"
    "Source/Core/World.cpp"
    "Source/Core/World.hpp"

    "Source/Graphics/D3D12/Buffer.cpp"
    "Source/Graphics/D3D12/Buffer.hpp"
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
    "Source/Graphics/D3D12/Texture.cpp"
    "Source/Graphics/D3D12/Texture.hpp"

    "Source/Graphics/Vertex.hpp"
    "Source/Graphics/Renderer.cpp"
    "Source/Graphics/Renderer.hpp"

    "Source/Platform/Windows/Window.cpp"
    "Source/Platform/Windows/Window.hpp"

    "Source/Rendering/RenderPass/ForwardLightingPass.cpp"
    "Source/Rendering/RenderPass/ForwardLightingPass.hpp"

    "Source/Rendering/Mesh.hpp"
    "Source/Rendering/MeshBuffer.cpp"
    "Source/Rendering/MeshBuffer.hpp"
    "Source/Rendering/MeshFactory.cpp"
    "Source/Rendering/MeshFactory.hpp"
    "Source/Rendering/RenderItem.hpp"

    "Source/Scene/Camera.cpp"
    "Source/Scene/Camera.hpp"
    "Source/Scene/CameraController.cpp"
    "Source/Scene/CameraController.hpp"
    "Source/Scene/Light.hpp"

    "Source/PCH.hpp"

    "${SHADER_GEN_DIR}/default_vs.h"
    "${SHADER_GEN_DIR}/default_ps.h"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${SHADER_GEN_DIR}")

target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX /permissive- /Zc:__cplusplus /sdl /utf-8)
else()
	target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)
endif()

target_precompile_headers(${PROJECT_NAME} PUBLIC "Source/PCH.hpp")

target_link_libraries(${PROJECT_NAME} PUBLIC d3d12 dxgi dxguid Microsoft::DirectX-Headers stb)

if(WIN32)
    target_link_options(${PROJECT_NAME} INTERFACE /ENTRY:mainCRTStartup)
endif()

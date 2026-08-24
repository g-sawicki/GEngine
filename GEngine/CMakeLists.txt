add_library(${PROJECT_NAME} STATIC)

# Shaders
include("${CMAKE_SOURCE_DIR}/cmake/ShaderCompiler.cmake")
set(SHADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets/Shaders")
compile_shader("default.hlsl" "VSMain" "vs_6_6" "default_vs.cso")
compile_shader("default.hlsl" "PSMain" "ps_6_6" "default_ps.cso")
compile_shader("shadow_pass.hlsl" "VSMain" "vs_6_6" "shadow_pass_vs.cso")
compile_shader("shadow_pass.hlsl" "PSMain" "ps_6_6" "shadow_pass_ps.cso")
compile_shader("tonemap.hlsl" "ToneMapCS" "cs_6_6" "tonemap_cs.cso")

set(GENGINE_SHADERS
    "${SHADER_GEN_DIR}/default_vs.cso"
    "${SHADER_GEN_DIR}/default_ps.cso"
    "${SHADER_GEN_DIR}/shadow_pass_vs.cso"
    "${SHADER_GEN_DIR}/shadow_pass_ps.cso"
    "${SHADER_GEN_DIR}/tonemap_cs.cso"
)
set(GENGINE_SHADERS "${GENGINE_SHADERS}" PARENT_SCOPE)

# Build the shaders as part of the default build; executables copy them at link time.
add_custom_target(GEngineShaders ALL DEPENDS ${GENGINE_SHADERS})
add_dependencies(GEngine GEngineShaders)

target_sources(${PROJECT_NAME} PRIVATE
    "Source/Core/Utility/Image.cpp"
    "Source/Core/Utility/Image.hpp"
    "Source/Core/Utility/Common.hpp"
    "Source/Core/Utility/MersenneTwister.hpp"
    
    "Source/Core/Application.cpp"
    "Source/Core/Application.hpp"
    "Source/Core/CommandLine.hpp"
    "Source/Core/Input.cpp"
    "Source/Core/Input.hpp"

    "Source/Graphics/D3D12/Buffer.cpp"
    "Source/Graphics/D3D12/Buffer.hpp"
    "Source/Graphics/D3D12/CommandList.cpp"
    "Source/Graphics/D3D12/CommandList.hpp"
    "Source/Graphics/D3D12/CommandQueue.cpp"
    "Source/Graphics/D3D12/CommandQueue.hpp"
    "Source/Graphics/D3D12/D3D12Common.hpp"
    "Source/Graphics/D3D12/DescriptorHeap.cpp"
    "Source/Graphics/D3D12/DescriptorHeap.hpp"
    "Source/Graphics/D3D12/Device.cpp"
    "Source/Graphics/D3D12/Device.hpp"
    "Source/Graphics/D3D12/Fence.cpp"
    "Source/Graphics/D3D12/Fence.hpp"
    "Source/Graphics/D3D12/PipelineState.cpp"
    "Source/Graphics/D3D12/PipelineState.hpp"
    "Source/Graphics/D3D12/RootSignature.cpp"
    "Source/Graphics/D3D12/RootSignature.hpp"
    "Source/Graphics/D3D12/Shader.cpp"
    "Source/Graphics/D3D12/Shader.hpp"
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
    "Source/Rendering/RenderPass/ShadowPass.cpp"
    "Source/Rendering/RenderPass/ShadowPass.hpp"
    "Source/Rendering/RenderPass/ToneMapPass.cpp"
    "Source/Rendering/RenderPass/ToneMapPass.hpp"

    "Source/Rendering/CascadedShadowMaps.cpp"
    "Source/Rendering/CascadedShadowMaps.hpp"
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
    "Source/Scene/Entity.hpp"
    "Source/Scene/EntityManager.cpp"
    "Source/Scene/EntityManager.hpp"
    "Source/Scene/Light.hpp"
    "Source/Scene/Material.hpp"
    "Source/Scene/Model.hpp"
    "Source/Scene/ModelLoader.cpp"
    "Source/Scene/ModelLoader.hpp"
    "Source/Scene/Scene.cpp"
    "Source/Scene/Scene.hpp"
    "Source/Scene/Transform.hpp"

    "Source/PCH.hpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_include_directories(${PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${agility_sdk_SOURCE_DIR}/build/native/include" # Agility SDK headers
)

target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX /permissive- /Zc:__cplusplus /sdl /utf-8)
else()
	target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)
endif()

target_precompile_headers(${PROJECT_NAME} PUBLIC "Source/PCH.hpp")

target_link_libraries(${PROJECT_NAME} PUBLIC d3d12 dxgi dxguid d3dcompiler Microsoft::DirectX-Headers stb assimp)

if(WIN32)
    target_link_options(${PROJECT_NAME} INTERFACE /ENTRY:mainCRTStartup)
endif()

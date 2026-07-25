add_executable(${PROJECT_NAME} WIN32)

target_sources(${PROJECT_NAME} PRIVATE
    "main.cpp"
    "Source/Graphics/D3D12/Device.cpp"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE d3d12 dxgi dxguid Microsoft::DirectX-Headers)

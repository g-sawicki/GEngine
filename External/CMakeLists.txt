include(FetchContent)

# DirectX-Headers
FetchContent_Declare(
    DirectX-Headers
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        v1.614.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(DirectX-Headers)

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")

# assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v6.0.5
    GIT_SHALLOW    TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# Agility SDK
FetchContent_Declare(
    agility_sdk
    URL "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.619.5"
    DOWNLOAD_NO_EXTRACT FALSE
)
FetchContent_MakeAvailable(agility_sdk)
set(agility_sdk_SOURCE_DIR "${agility_sdk_SOURCE_DIR}" CACHE INTERNAL "Agility SDK Source Directory")

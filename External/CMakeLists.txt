include(FetchContent)

FetchContent_Declare(
    DirectX-Headers
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
    GIT_TAG        v1.614.1
)
FetchContent_MakeAvailable(DirectX-Headers)

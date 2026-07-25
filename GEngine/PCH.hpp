#pragma once

// Windows
#include <Windows.h>
#include <shellapi.h>

// Fix macro collisions with Win32
#if defined(CreateWindow)
#undef CreateWindow
#endif

// WRL
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <directx/d3dx12.h>

// C++ standard library
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <stdexcept>

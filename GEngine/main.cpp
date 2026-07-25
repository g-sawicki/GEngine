#include <Windows.h>
#include <shellapi.h>

#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
//#include <d3dx12.h>

#include <iostream>

int main()
{
	std::printf("Hello World");
	return 0;
}

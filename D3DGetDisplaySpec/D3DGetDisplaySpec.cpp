/* MIT License

Copyright(c) 2017  Masafumi Takahashi / http://www.shader.jp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "stdafx.h"

#include <iostream>
#include <string>
#include <exception>

// Global
UINT g_dxgiFactoryFlags;
Microsoft::WRL::ComPtr<IDXGIFactory4> g_factory;
Microsoft::WRL::ComPtr<IDXGISwapChain4> g_swapChain;
Microsoft::WRL::ComPtr<ID3D12Device> g_device;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_commandQueue;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

void InitDXGIAndDevice()
{
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			g_dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(g_dxgiFactoryFlags, IID_PPV_ARGS(&g_factory)));

	Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(g_factory.Get(), &hardwareAdapter);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_device)
	));

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue)));
}

void CreateSwapChain()
{
	uint32_t FrameCount = 2;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = 640;
	swapChainDesc.Height = 480;
	swapChainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(g_factory->CreateSwapChainForHwnd(
		g_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		GetConsoleWindow(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	ThrowIfFailed(swapChain.As(&g_swapChain));
}

void GetDisplayInformation(IDXGISwapChain4* swapChain, DXGI_OUTPUT_DESC1& outputDesc)
{
	// Get information about the display we are presenting to.
	Microsoft::WRL::ComPtr<IDXGIOutput> output;
	Microsoft::WRL::ComPtr<IDXGIOutput6> output6;

	ThrowIfFailed(g_swapChain->GetContainingOutput(&output));
	if (SUCCEEDED(output.As(&output6)))
	{
		ThrowIfFailed(output6->GetDesc1(&outputDesc));	}
	else
	{
		std::cout << "Fail : Get IDXGIOutput6" << std::endl;
	}

}

std::string GetColorSpaceString(DXGI_COLOR_SPACE_TYPE colorSpace)
{
	std::string colorSpaceString;
	switch (colorSpace)
	{
	case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709";
		break;
	case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709";
		break;
	case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709";
		break;
	case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020";
		break;
	case DXGI_COLOR_SPACE_RESERVED:
		colorSpaceString = "DXGI_COLOR_SPACE_RESERVED";
		break;
	case DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601";
		break;
	case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709";
		break;
	case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:
		colorSpaceString = "DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020";
		break;
	case DXGI_COLOR_SPACE_CUSTOM:
		colorSpaceString = "DXGI_COLOR_SPACE_CUSTOM";
		break;
	default:
		break;
	}

	return colorSpaceString;
}

int main()
{
	InitDXGIAndDevice();
	CreateSwapChain();

	//
	DXGI_OUTPUT_DESC1 outputDesc;
	GetDisplayInformation(g_swapChain.Get(), outputDesc);

	// Device Name
	std::wstring displayName = outputDesc.DeviceName;
	std::wcout << L"Device Name: " << displayName << std::endl;

	// Color Space
	std::string colorSpace = GetColorSpaceString(outputDesc.ColorSpace);
	std::cout << "Color Space: " << colorSpace << std::endl;

	// RedPrimary
	std::cout << "RedPrimary: " << outputDesc.RedPrimary[0] << " " << outputDesc.RedPrimary[1] << std::endl;
	// GreenPrimary
	std::cout << "GreenPrimary: " << outputDesc.GreenPrimary[0] << " " << outputDesc.GreenPrimary[1] << std::endl;
	// BluePrimary
	std::cout << "BluePrimary: " << outputDesc.BluePrimary[0] << " " << outputDesc.BluePrimary[1] << std::endl;
	// WhitePoint
	std::cout << "WhitePoint: " << outputDesc.WhitePoint[0] << " " << outputDesc.WhitePoint[1] << std::endl;

	// MinLuminance
	std::cout << "MinLuminance: " << outputDesc.MinLuminance << std::endl;
	// MaxLuminance
	std::cout << "MaxLuminance: " << outputDesc.MaxLuminance << std::endl;
	// MaxFullFrameLuminance
	std::cout << "MaxFullFrameLuminance: " << outputDesc.MaxFullFrameLuminance << std::endl;

	// BitsPerColor
	std::cout << "BitsPerColor: " << outputDesc.BitsPerColor << std::endl;

    return 0;
}


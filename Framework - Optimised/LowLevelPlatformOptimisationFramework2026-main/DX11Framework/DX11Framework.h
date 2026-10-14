#pragma once

#include <windows.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <string>
#include <array>
#include <list>
#include <memory>
#include "GameObject.h"
#include "ScanCode.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct ConstantBuffer
{
	XMMATRIX Projection;
	XMMATRIX View;
	XMMATRIX World;
};

class DX11Framework
{
	int _WindowWidth = 1280;
	int _WindowHeight = 768;

	ComPtr<ID3D11DeviceContext> _immediateContext;
	ComPtr<ID3D11Device> _device;
	ComPtr<IDXGIDevice> _dxgiDevice;
	ComPtr<IDXGIFactory2> _dxgiFactory;
	ComPtr<ID3D11RenderTargetView> _frameBufferView;
	ComPtr<IDXGISwapChain1> _swapChain;
	D3D11_VIEWPORT _viewport;

	ComPtr<ID3D11RasterizerState> _rasterizerState;
	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11PixelShader> _pixelShader;
	ComPtr<ID3D11Buffer> _constantBuffer;
	ComPtr<ID3D11Texture2D> _depthStencilBuffer;
	ComPtr<ID3D11DepthStencilView> _depthStencilView;
	ComPtr<ID3D11DepthStencilState> _depthStencilState;

	HWND _windowHandle;

	XMFLOAT4X4 _View;
	XMFLOAT4X4 _Projection;

	ConstantBuffer _cbData;

	std::list<std::unique_ptr<GameObject>> _objects;

public:
	void Initialise(HINSTANCE hInstance, int nCmdShow);
	void CreateWindowHandle(HINSTANCE hInstance, int nCmdShow);
	void CreateD3DDevice();
	void CreateSwapChainAndFrameBuffer();
	void CompileShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile, ComPtr<ID3DBlob>& shaderBlob) const;
	void InitShadersAndInputLayout();
	void InitPipelineVariables();
	void InitRunTimeData();
	void InitImGui();

	~DX11Framework();

	LRESULT HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void Simulate(float deltaTime);
	void Draw();
	void SetUpUI() const;
	void RenderUI();
};
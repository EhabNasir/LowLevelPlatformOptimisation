#include "DX11Framework.h"

#include "Util.h"
#include "IMGUI/imgui.h"
#include "IMGUI/imgui_impl_dx11.h"
#include "IMGUI/imgui_impl_win32.h"
#include "Box.h"
#include "Floor.h"

#include<optick.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr size_t boxCount = 512;

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
            return true;

        DX11Framework* application;

        switch (message)
        {
        case WM_CREATE: //WindowProc needs to be static for CreateWindow(), we pass a DX11Framework pointer as an additional parameter
        {               //We reintepret the window creation information into a DX11Framework pointer
            application = static_cast<DX11Framework*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
            //Now add that pointer as some extra data associated with our window
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        }
        return 0;
        }

        //If not creating, we should have already stored the correct instance pointer, so now get it and call it
        application = reinterpret_cast<DX11Framework*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        return application->HandleMessage(hWnd, message, wParam, lParam);
    }
}

LRESULT DX11Framework::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    switch (message)
    {
    case WM_KEYDOWN:
        //OnKeyDown
        return 0;

    case WM_KEYUP:
        //OnKeyUp
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void DX11Framework::Initialise(HINSTANCE hInstance, int nShowCmd)
{
    CreateWindowHandle(hInstance, nShowCmd);
    CreateD3DDevice();
    CreateSwapChainAndFrameBuffer();
    InitShadersAndInputLayout();
    InitImGui();
    InitPipelineVariables();
    InitRunTimeData();
}

void DX11Framework::CreateWindowHandle(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t* windowName  = L"DX11Framework";

    WNDCLASSW wndClass;
    wndClass.style = 0;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = 0;
    wndClass.hIcon = 0;
    wndClass.hCursor = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = windowName;

    RegisterClassW(&wndClass);

    _windowHandle = CreateWindowExW(0, windowName, windowName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        _WindowWidth, _WindowHeight, nullptr, nullptr, hInstance, this);

}

void DX11Framework::CreateD3DDevice()
{
    ThrowOnFail(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory)));

    constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;

    DWORD createDeviceFlags = 0;
#ifdef _DEBUG
    // If we're building for Debug, contain device warnings and errors
    // These are output to the visual studio console
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ThrowOnFail(D3D11CreateDevice(nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | createDeviceFlags,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        _device.GetAddressOf(),
        nullptr,
        _immediateContext.GetAddressOf()));
}

void DX11Framework::CreateSwapChainAndFrameBuffer()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = _WindowWidth;
    swapChainDesc.Height = _WindowHeight;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    ThrowOnFail(_dxgiFactory->CreateSwapChainForHwnd(_device.Get(), _windowHandle, &swapChainDesc, nullptr, nullptr, _swapChain.GetAddressOf()));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ComPtr<ID3D11Texture2D> frameBuffer = nullptr;

    ThrowOnFail(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &frameBuffer));

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    ThrowOnFail(_device->CreateRenderTargetView(frameBuffer.Get(), &framebufferDesc, _frameBufferView.GetAddressOf()));

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = _WindowWidth;
    depthDesc.Height = _WindowHeight;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ThrowOnFail(_device->CreateTexture2D(&depthDesc, nullptr, _depthStencilBuffer.GetAddressOf()));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    ThrowOnFail(_device->CreateDepthStencilView(_depthStencilBuffer.Get(), &dsvDesc, _depthStencilView.GetAddressOf()));
}

void DX11Framework::CompileShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile, ComPtr<ID3DBlob>& shaderBlob) const
{
    ComPtr<ID3DBlob> errorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag and D3DCOMPILE_SKIP_OPTIMIZATION
    // This means the shader runs slower, but debugging is easier
    // As optimizations will move/remove lines of code in the shader
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/d3dcompile-constants
    dwShaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr;
    hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(), dwShaderFlags, 0, &shaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        //Inspect msg or check VS Output to see compilation error
        std::string msg((char*)errorBlob->GetBufferPointer());
        OutputDebugStringA(msg.c_str());
        throw std::exception();
    }
}

void DX11Framework::InitShadersAndInputLayout()
{    
    ComPtr<ID3DBlob> vertexShaderBlob;

    CompileShader(L"SimpleShaders.hlsl", "VS_main", "vs_5_0", vertexShaderBlob);
    ThrowOnFail(_device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, _vertexShader.GetAddressOf()));

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
    };

    ThrowOnFail(_device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), _inputLayout.GetAddressOf()));
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ComPtr<ID3DBlob> pixelShaderBlob;
    CompileShader(L"SimpleShaders.hlsl", "PS_main", "ps_5_0", pixelShaderBlob);

    ThrowOnFail(_device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, _pixelShader.GetAddressOf()));
}

//ImGui is a simple UI framework for development, it is not designed for release quality UI
void DX11Framework::InitImGui()
{
    //This project only contains ImGui implementations for Win32 and DirectX
    //ImGui works across many backends, for full specs see https://github.com/ocornut/imgui/
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    //ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(_windowHandle);
    ImGui_ImplDX11_Init(_device.Get(), _immediateContext.Get());
}

void DX11Framework::InitPipelineVariables()
{
    //Input Assembler
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _immediateContext->IASetInputLayout(_inputLayout.Get());

    //Rasterizer
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    ThrowOnFail(_device->CreateRasterizerState(&rasterizerDesc, _rasterizerState.GetAddressOf()));
    _immediateContext->RSSetState(_rasterizerState.Get());

    //Viewport Values
    _viewport = { 0.0f, 0.0f, (float)_WindowWidth, (float)_WindowHeight, 0.0f, 1.0f };
    _immediateContext->RSSetViewports(1, &_viewport);

    //Constant Buffer
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowOnFail(_device->CreateBuffer(&constantBufferDesc, nullptr, _constantBuffer.GetAddressOf()));

    _immediateContext->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
    _immediateContext->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    ThrowOnFail(_device->CreateDepthStencilState(&depthStencilDesc, _depthStencilState.GetAddressOf()));
    _immediateContext->OMSetDepthStencilState(_depthStencilState.Get(), 0);
}

void DX11Framework::InitRunTimeData()
{
    //Camera
    XMFLOAT3 Eye = XMFLOAT3(0, 10, -50.0f);
    XMFLOAT3 At = XMFLOAT3(0, 0, 0);
    XMFLOAT3 Up = XMFLOAT3(0, 1, 0);

    XMStoreFloat4x4(&_View, XMMatrixLookAtLH(XMLoadFloat3(&Eye), XMLoadFloat3(&At), XMLoadFloat3(&Up)));

    //Projection
    float aspect = _viewport.Width / _viewport.Height;
    XMMATRIX perspective = XMMatrixPerspectiveFovLH(XMConvertToRadians(90), aspect, 0.01f, 100.0f);
    XMStoreFloat4x4(&_Projection, perspective);

    _objects.push_back(std::make_unique<Floor>(_device.Get(), _pixelShader, _vertexShader, Vector3
    {
        .x = 0,
        .y = 0,
        .z = 0,
    }));

    for (int i = 0; i < boxCount; i += 1)
    {
        _objects.push_back(std::make_unique<Box>(_device.Get(), _pixelShader, _vertexShader, Vector3
        {
            .x = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 20.0f)),
            .y = 10.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 1.0f)),
            .z = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 20.0f)),
        }));
    }
}

DX11Framework::~DX11Framework()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void DX11Framework::Simulate(float deltaTime)
{
    OPTICK_EVENT("Simulate");

    for (std::unique_ptr<GameObject> & object : _objects)
    {
        if (PhysicsObject * physics = object->GetPhysics())
        {
            physics->Impulse({ 0.f, gravity * deltaTime, 0.f });
            physics->Simulate(_objects, deltaTime);
        }
    }
}

void DX11Framework::Draw()
{    
    OPTICK_EVENT("Draw");

    SetUpUI();

    //Present unbinds render target, so rebind and clear at start of each frame
    float backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

    _immediateContext->OMSetRenderTargets(1, _frameBufferView.GetAddressOf(), _depthStencilView.Get());
    _immediateContext->ClearRenderTargetView(_frameBufferView.Get(), backgroundColor);
    _immediateContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    XMMatrixTranspose(XMLoadFloat4x4(&_View));

    _cbData.View = XMMatrixTranspose(XMLoadFloat4x4(&_View));
    _cbData.Projection = XMMatrixTranspose(XMLoadFloat4x4(&_Projection));

    for (std::unique_ptr<GameObject> & object : _objects)
    {
        _cbData.World = XMMatrixTranspose(XMMatrixTranslation(object->Position().x, object->Position().y, object->Position().z));

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;

        _immediateContext->Map(_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
        memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
        _immediateContext->Unmap(_constantBuffer.Get(), 0);
        object->Render(_immediateContext.Get());
    }

    RenderUI();
    _swapChain->Present(0, 0);
}

void DX11Framework::SetUpUI() const
{
    //Can be done at any point in a frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Simple Metrics");
    ImGui::End();
}

void DX11Framework::RenderUI()
{
    // Must be done after drawing objects and before present
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
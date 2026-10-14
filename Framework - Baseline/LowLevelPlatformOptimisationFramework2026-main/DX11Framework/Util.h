#pragma once

#include <Windows.h>
#include <exception>
#include <d3d11_4.h>
#include <DirectXMath.h>

struct SimpleVertex final
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

inline void ThrowOnFail(HRESULT x)
{
    if (FAILED(x))
        //Check VS Output for error and call stack for location of error
        throw std::exception();
}


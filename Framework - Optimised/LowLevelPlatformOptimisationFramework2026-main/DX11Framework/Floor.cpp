#include "Floor.h"

#include <array>
#include <DirectXMath.h>
#include "Util.h"

using DirectX::XMFLOAT3;

namespace
{
    constexpr std::array boxVertices = std::to_array<SimpleVertex>(
    {
        // Position                     // Color             
        { XMFLOAT3{ -20.f, 0.f, -20.f }, XMFLOAT3{ 1.f, 1.f, 1.f } },
        { XMFLOAT3{ -20.f, 0.f,  20.f }, XMFLOAT3{ 1.f, 1.f, 1.f } },
        { XMFLOAT3{  20.f, 0.f,  20.f }, XMFLOAT3{ 1.f, 1.f, 1.f } },
        { XMFLOAT3{  20.f, 0.f, -20.f }, XMFLOAT3{ 1.f, 1.f, 1.f } },
    });

    constexpr std::array const boxIndices = std::to_array<WORD>(
    {
        0, 1, 2, 0, 2, 3,
    });
}

Floor::Floor(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader, Vector3 position)
{
    _extents = { 20, 20 };
    _pixelShader = pixelShader;
    _vertexShader = vertexShader;

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(SimpleVertex) * boxVertices.size());
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA const vertexData = { boxVertices.data() };

    ThrowOnFail(device->CreateBuffer(&vertexBufferDesc, &vertexData, _vertexBuffer.GetAddressOf()));

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(WORD) * boxIndices.size());
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = { boxIndices.data() };

    ThrowOnFail(device->CreateBuffer(&indexBufferDesc, &indexData, _indexBuffer.GetAddressOf()));
}

bool Floor::IsIntersecting(const Box::Collider & box) const
{
    float const minBoxY = box.Position().y - box.Extents().y;
    float const maxBoxY = box.Position().y + box.Extents().y;

    if (Position().y < minBoxY || Position().y > maxBoxY)
    {
        return false;
    }

    float const xDistance = std::abs(box.Position().x - Position().x);
    float const xDistanceMax = box.Extents().x + _extents.x;

    if (xDistance > xDistanceMax)
    {
        return false;
    }

    float const zDistance = std::abs(box.Position().z - Position().z);
    float const zDistanceMax = box.Extents().z + _extents.y;

    if (zDistance > zDistanceMax)
    {
        return false;
    }

    return true;
}

void Floor::Render(ID3D11DeviceContext * context)
{
    UINT stride = { sizeof(SimpleVertex) };
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->VSSetShader(_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(_pixelShader.Get(), nullptr, 0);
    context->DrawIndexed(static_cast<UINT>(boxIndices.size()), 0, 0);
}

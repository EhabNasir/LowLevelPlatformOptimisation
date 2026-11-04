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

bool Floor::IsIntersecting(Vector3 position, Vector3 extents) const
{
    float const minBoxY = position.y - extents.y;
    float const maxBoxY = position.y + extents.y;

    if (Position().y < minBoxY || Position().y > maxBoxY)
        return false;

    if (std::abs(position.x - Position().x) > extents.x + _extents.x)
        return false;

    if (std::abs(position.z - Position().z) > extents.z + _extents.y)
        return false;

    return true;
}

bool Floor::IsIntersecting(const Box::Collider& box) const
{
    return IsIntersecting(box.Position(), box.Extents());
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

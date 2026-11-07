#include "Box.h"

#include "Floor.h"
#include "Util.h"

#include <array>
#include <cmath>
#include <DirectXMath.h>
#include<cassert>
#include <vector>

using DirectX::XMFLOAT3;

namespace
{
    constexpr std::array boxVertices = std::to_array<SimpleVertex>(
    {
        // Position                     // Color             
        { XMFLOAT3{ -1.f, -1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 0.f } },
        { XMFLOAT3{ -1.f,  1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 0.f } },
        { XMFLOAT3{  1.f,  1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 0.f } },
        { XMFLOAT3{  1.f, -1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 0.f } },

        { XMFLOAT3{ -1.f, -1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 0.f } },
        { XMFLOAT3{  1.f, -1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 0.f } },
        { XMFLOAT3{  1.f,  1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 0.f } },
        { XMFLOAT3{ -1.f,  1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 0.f } },

        { XMFLOAT3{ -1.f, -1.f, 1.f },  XMFLOAT3{ 0.f, 0.f, 1.f } },
        { XMFLOAT3{ -1.f,  1.f, 1.f },  XMFLOAT3{ 0.f, 0.f, 1.f } },
        { XMFLOAT3{ -1.f,  1.f, -1.f }, XMFLOAT3{ 0.f, 0.f, 1.f } },
        { XMFLOAT3{ -1.f, -1.f, -1.f }, XMFLOAT3{ 0.f, 0.f, 1.f } },

        { XMFLOAT3{ 1.f, -1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 1.f } },
        { XMFLOAT3{ 1.f,  1.f, -1.f }, XMFLOAT3{ 1.f, 0.f, 1.f } },
        { XMFLOAT3{ 1.f,  1.f,  1.f }, XMFLOAT3{ 1.f, 0.f, 1.f } },
        { XMFLOAT3{ 1.f, -1.f,  1.f }, XMFLOAT3{ 1.f, 0.f, 1.f } },

        { XMFLOAT3{ -1.f, 1.f, -1.f }, XMFLOAT3{ 1.f, 1.f, 0.f } },
        { XMFLOAT3{ -1.f, 1.f,  1.f }, XMFLOAT3{ 1.f, 1.f, 0.f } },
        { XMFLOAT3{  1.f, 1.f,  1.f }, XMFLOAT3{ 1.f, 1.f, 0.f } },
        { XMFLOAT3{  1.f, 1.f, -1.f }, XMFLOAT3{ 1.f, 1.f, 0.f } },

        { XMFLOAT3{ -1.f,-1.f,-1.f },  XMFLOAT3{ 0.f, 1.f, 1.f } },
        { XMFLOAT3{  1.f,-1.f,-1.f },  XMFLOAT3{ 0.f, 1.f, 1.f } },
        { XMFLOAT3{  1.f,-1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 1.f } },
        { XMFLOAT3{ -1.f,-1.f, 1.f },  XMFLOAT3{ 0.f, 1.f, 1.f } },
    });

    constexpr std::array const boxIndices = std::to_array<WORD>(
    {
        0,  1,  2,  0,  2,  3,
        4,  5,  6,  4,  6,  7,
        8,  9,  10, 8,  10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    });

    //constexpr size_t poolSlots = 1024;
    //constexpr size_t slotsPerChunk = 128;

    //size_t& BoxFallbacks() { thread_local size_t count = 0; return count; }
    //size_t& ColliderFallbacks() { thread_local size_t count = 0; return count; }


    //FixedPool& BoxPool()
    //{
    //    thread_local FixedPool pool(sizeof(Box), slotsPerChunk);
    //    return pool;
    //}

    //FixedPool& ColliderPool()
    //{
    //    thread_local FixedPool pool(sizeof(Box::Collider), slotsPerChunk);
    //    return pool;
    //}
}

bool Box::Collider::CheckCollision(const Collider & that) const
{
    return (std::abs(Position().x - that.Position().x) * 2 < (_extents.x + that._extents.x)) &&
        (std::abs(Position().y - that.Position().y) * 2 < (_extents.y + that._extents.y)) &&
        (std::abs(Position().z - that.Position().z) * 2 < (_extents.z + that._extents.z));
}

void Box::Collider::ResolveCollision(Collider & that)
{
    Vector3 const distance = { Position().x - that.Position().x, Position().y - that.Position().y, Position().z - that.Position().z};
    float const length = std::sqrt(distance.x * distance.x + distance.y * distance.y + distance.z * distance.z);
    Vector3 const normal = { distance.x / length, distance.y / length, distance.z / length };

    Vector3 const relativeVelocity =
    {
        .x = Velocity().x - that.Velocity().x,
        .y = Velocity().y - that.Velocity().y,
        .z = Velocity().z - that.Velocity().z,
    };

    // Relative velocity along the normal.
    float impulse = relativeVelocity.x * normal.x + relativeVelocity.y * normal.y + relativeVelocity.z * normal.z;

    // Ignore if objects are already moving away.
    if (impulse > 0)
    {
        return;
    }

    float e = 0.01f; // Coefficient of restitution (0 = inelastic, 1 = elastic)
    float dampening = 0.9f; // Dampening factor (0.9 = 10% energy reduction)
    float j = -(1.0f + e) * impulse * dampening;

    Impulse({j * normal.x, j * normal.y, j * normal.z});
    that.Impulse({j * -normal.x, j * -normal.y, j * -normal.z});
}

void Box::Collider::Simulate(std::vector<std::unique_ptr<GameObject>>& objects, float deltaTime)
{
    if (!useLocking)
    {
        PhysicsObject::Simulate(objects, deltaTime);

        if (_floor && _floor->IsIntersecting(*this))
            Impulse({ 0.f, -Velocity().y, 0.f });

        for (std::unique_ptr<GameObject>& object : objects)
        {
            auto const collider = static_cast<Collider*>(object->GetPhysics());
            if (collider && collider != this && CheckCollision(*collider))
                return ResolveCollision(*collider);
        }
        return;
    }

    {
        std::scoped_lock lock(Mutex());
        PhysicsObject::Simulate(objects, deltaTime);

        if (_floor && _floor->IsIntersecting(*this))
            Impulse({ 0.f, -Velocity().y, 0.f });
    }

    for (std::unique_ptr<GameObject>& object : objects)
    {
        auto const collider = static_cast<Collider*>(object->GetPhysics());
        if (!collider || collider == this)
            continue;

        std::scoped_lock lock(Mutex(), collider->Mutex());

        if (CheckCollision(*collider))
        {
            ResolveCollision(*collider);
            return;
        }
    }
}
Box::Box(ID3D11Device * device, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader, Vector3 position)
{
    _collider = std::make_unique<Collider>(position, Vector3{1, 1, 1});
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

void Box::Render(ID3D11DeviceContext * context)
{
    UINT stride = { sizeof(SimpleVertex) };
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->VSSetShader(_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(_pixelShader.Get(), nullptr, 0);
    context->DrawIndexed(static_cast<UINT>(boxIndices.size()), 0, 0);
}

//void* Box::operator new(size_t size)
//{
//    return ::operator new(size);
//}

//void* Box::operator new(size_t size)
//{
//    assert(size == sizeof(Box));
//    return ::operator new(size);
//}

//void Box::operator delete(void* pointer, size_t size)
//{
//    ::operator delete(pointer, size);
//}

//void* Box::Collider::operator new(size_t size)
//{
//    return ::operator new(size);
//}

//void* Box::Collider::operator new(size_t size)
//{
//    assert(size == sizeof(Box::Collider));
//    return ::operator new(size);
//}

//void Box::Collider::operator delete(void* pointer, size_t size)
//{
//    ::operator delete(pointer, size);
//}

//void* Box::operator new(size_t size)
//{
//    assert(size == sizeof(Box));
//    if (void* pointer = BoxPool().Allocate())
//        return pointer;
//    ++BoxFallbacks();
//    return ::operator new(size);
//}
//
//void Box::operator delete(void* pointer, size_t size)
//{
//    if (!pointer)
//        return;
//    if (BoxPool().Contains(pointer))
//        BoxPool().Free(pointer);
//    else
//        ::operator delete(pointer, size);
//}
//
//void* Box::Collider::operator new(size_t size)
//{
//    assert(size == sizeof(Box::Collider));
//    if (void* pointer = ColliderPool().Allocate())
//        return pointer;
//    ++ColliderFallbacks();
//    return ::operator new(size);
//}
//
//void Box::Collider::operator delete(void* pointer, size_t size)
//{
//    if (!pointer)
//        return;
//    if (ColliderPool().Contains(pointer))
//        ColliderPool().Free(pointer);
//    else
//        ::operator delete(pointer, size);
//}
//
//size_t Box::FallbackCount() { return BoxFallbacks(); }
//size_t Box::PoolChunks() { return BoxPool().ChunkCount(); }
//
//size_t Box::Collider::FallbackCount() { return ColliderFallbacks(); }
//size_t Box::Collider::PoolChunks() { return ColliderPool().ChunkCount(); }
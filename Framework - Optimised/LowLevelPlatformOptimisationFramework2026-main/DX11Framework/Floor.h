#pragma once

#include "GameObject.h"
#include "Box.h"

struct Floor final : public GameObject
{
	explicit Floor(ID3D11Device * device, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader, Vector3 position);

	bool IsIntersecting(Box::Collider) const;

	PhysicsObject* GetPhysics() override { return nullptr; }

	Vector3 Position() const override { return { 0, 0, 0 }; }

	void Render(ID3D11DeviceContext * context) override;

	private:
	Vector2 _extents;

	Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
};

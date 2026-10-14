#pragma once

#include <d3d11_4.h>
#include <wrl.h>
#include <memory>
#include "GameObject.h"

struct Box final : public GameObject
{
	struct Collider final : public PhysicsObject
	{
		explicit Collider(Vector3 position, Vector3 extents) : PhysicsObject{ position }, _extents { extents } {}

		Vector3 Extents() const { return _extents; }

		void Simulate(std::list<std::unique_ptr<GameObject>> & objects, float delatTime) override;

		private:
		Vector3 _extents;

		bool CheckCollision(Collider that) const;

		void ResolveCollision(Collider & that);
	};

	explicit Box(ID3D11Device *, Microsoft::WRL::ComPtr<ID3D11PixelShader>, Microsoft::WRL::ComPtr<ID3D11VertexShader>, Vector3);

	Vector3 Position() const { return _collider->Position(); }

	PhysicsObject * GetPhysics() override { return _collider.get(); }

	void Render(ID3D11DeviceContext *) override;

	private:
	std::unique_ptr<Collider> _collider;

	Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
};
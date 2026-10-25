#pragma once

#include <d3d11_4.h>
#include <list>
#include <memory>
#include <vector>

constexpr float gravity = -.98f;

struct Vector2 final
{
	float x = 0.f, y = 0.f;
};

struct Vector3 final
{
	float x = 0.f, y = 0.f, z = 0.f;
};

struct PhysicsObject;

struct GameObject
{
	virtual ~GameObject() {}

	virtual Vector3 Position() const = 0;

	virtual void Render(ID3D11DeviceContext *) = 0;

	virtual PhysicsObject* GetPhysics() = 0;
};

struct PhysicsObject
{
	explicit PhysicsObject(Vector3 position) : _position{ position } {}

	virtual void Simulate(std::vector<std::unique_ptr<GameObject>> & objects, float deltaTime)
	{
		_position.x += _velocity.x * deltaTime;
		_position.y += _velocity.y * deltaTime;
		_position.z += _velocity.z * deltaTime;
	}

	void Impulse(Vector3 direction)
	{
		_velocity.x += direction.x;
		_velocity.y += direction.y;
		_velocity.z += direction.z;
	}

	Vector3 Position() const { return _position; }

	Vector3 Velocity() const { return _velocity; }

	private:
	Vector3 _position;

	Vector3 _velocity;
};

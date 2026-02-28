#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Transform.h"

class Camera
{
private:
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;

	std::shared_ptr<Transform> transform;

	float aspectRatio;
	float fov;
	float nearPlane;
	float farPlane;
	float movementSpeed;
	float lookSpeed;
	bool isPerspective;

public:
	Camera(
		float aspectRatio,
		DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT3 rot = DirectX::XMFLOAT3(0, 0, 0),
		float movementSpeed = 1.0f,
		float lookSpeed = 0.01f,
		bool isPerspective = true,
		float fov = DirectX::XM_PIDIV2,
		float nearPlane = 0.1f,
		float farPlane = 100.0f
	);

	// Getters
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();
	std::shared_ptr<Transform> GetTransform();

	float GetFov();
	float GetNearPlane();
	float GetFarPlane();
	float GetMovementSpeed();
	float GetLookSpeed();
	
	// Setters
	void SetFov(float fov);
	void SetNearPlane(float nearPlane);
	void SetFarPlane(float farPlane);
	void SetMovementSpeed(float movementSpeed);
	void SetLookSpeed(float lookSpeed);

	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);
	void Update(float dt);
};


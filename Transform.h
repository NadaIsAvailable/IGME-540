#pragma once

#include <DirectXMath.h>

class Transform
{
private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll;
	DirectX::XMFLOAT3 scale;

	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;

	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInverseTranspose;

	bool matrixDirty;
	bool vectorDirty;

public:
	Transform();

	// Setters
	void SetPosition(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetRotation(float pitch = 0.0f, float yaw = 0.0f, float roll = 0.0f);
	void SetRotation(DirectX::XMFLOAT3 rotation);	// XMFLOAT4 for quaternion
	void SetScale(float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void SetScale(DirectX::XMFLOAT3 scale);

	// Getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll(); // XMFLOAT4 GetRotation() for quaternion
	DirectX::XMFLOAT3 GetScale();

	DirectX::XMFLOAT3 GetForward();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();

	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	// Transformers
	void MoveAbsolute(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void MoveRelative(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch = 0.0f, float yaw = 0.0f, float roll = 0.0f);
	void Rotate(DirectX::XMFLOAT3 rotation);
	void Scale(float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void Scale(DirectX::XMFLOAT3 scale);

private: 
	// Calulation helper methods
	void CalculateWorldMatrix();
	void CalculateVectors();
};


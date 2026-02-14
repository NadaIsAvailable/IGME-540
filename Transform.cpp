#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
	position(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1, 1, 1),
	dirty(true)
{
	XMStoreFloat4x4(&world, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());
}

void Transform::SetPosition(float x, float y, float z)
{
	SetPosition(XMFLOAT3(x, y, z));
}

void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	// Load the position into an XMVECTOR and store it back in the XMFLOAT3
	XMStoreFloat3(&this->position, XMLoadFloat3(&position));
	dirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	SetRotation(XMFLOAT3(pitch, yaw, roll));
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	// Load the rotation into an XMVECTOR and store it back in the XMFLOAT3
	XMStoreFloat3(&this->pitchYawRoll, XMLoadFloat3(&rotation));
	dirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	// Load the scale into an XMVECTOR and store it back in the XMFLOAT3
	XMStoreFloat3(&this->scale, XMLoadFloat3(&scale));
	dirty = true;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if (dirty) CalculateWorldMatrix();
	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	if (dirty) CalculateWorldMatrix();
	return worldInverseTranspose;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	MoveAbsolute(XMFLOAT3(x, y, z));
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	// Load the current position into an XMVECTOR, 
	// add the offset to it 
	// and store it back in the XMFLOAT3
	XMStoreFloat3(
		&this->position,
		XMVectorAdd(
			XMLoadFloat3(&this->position),
			XMLoadFloat3(&offset)
		)
	);

	dirty = true;
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
	Rotate(XMFLOAT3(pitch, yaw, roll));
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	// Load the current rotation into an XMVECTOR,
	// add the rotation to it
	// and store it back in the XMFLOAT3
	XMStoreFloat3(
		&this->pitchYawRoll,
		XMVectorAdd(
			XMLoadFloat3(&this->pitchYawRoll),
			XMLoadFloat3(&rotation)
		)
	);

	dirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	Scale(XMFLOAT3(x, y, z));
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	// Load the current scale into an XMVECTOR,
	// multiply it with the scale
	// and store it back in the XMFLOAT3
	XMStoreFloat3(
		&this->scale,
		XMVectorMultiply(
			XMLoadFloat3(&this->scale),
			XMLoadFloat3(&scale)
		)
	);

	dirty = true;
}

void Transform::CalculateWorldMatrix()
{
	// Create translation, rotation and scale matrices from corresponding vectors
	XMMATRIX t = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
	XMMATRIX s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

	// Combine the above matrices to get the world matrix
	XMMATRIX w = XMMatrixMultiply(XMMatrixMultiply(s, r), t);

	// Store the result in the world matrix
	XMStoreFloat4x4(&world, w);

	// Calculate the inverse transpose of the world matrix and store it
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(w)));

	// Reset the dirty flag
	dirty = false;
}

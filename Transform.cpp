#include "Transform.h"

using namespace DirectX;

Transform::Transform() :
	position(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1, 1, 1),
	forward(0, 0, 1),
	right(1, 0, 0),
	up(0, 1, 0),
	matrixDirty(true),
	vectorDirty(true)
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
	matrixDirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	SetRotation(XMFLOAT3(pitch, yaw, roll));
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	// Load the rotation into an XMVECTOR and store it back in the XMFLOAT3
	XMStoreFloat3(&this->pitchYawRoll, XMLoadFloat3(&rotation));
	matrixDirty = true;
	vectorDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	// Load the scale into an XMVECTOR and store it back in the XMFLOAT3
	XMStoreFloat3(&this->scale, XMLoadFloat3(&scale));
	matrixDirty = true;
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

DirectX::XMFLOAT3 Transform::GetForward()
{
	if (vectorDirty) CalculateVectors();
	return forward;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
	if (vectorDirty) CalculateVectors();
	return right;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
	if (vectorDirty) CalculateVectors();
	return up;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if (matrixDirty) CalculateWorldMatrix();
	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	if (matrixDirty) CalculateWorldMatrix();
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

	matrixDirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
	MoveRelative(XMFLOAT3(x, y, z));
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	// Create a quaternion representing the current rotation
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Rotate the offset with the rotation quaternion
	XMVECTOR rotatedOffset = XMVector3Rotate(XMLoadFloat3(&offset), rotQuat);

	// Add the rotated offset to the current position
	// and store back in the position vector
	XMStoreFloat3(
		&position,
		XMVectorAdd(
			XMLoadFloat3(&position),
			rotatedOffset
		)
	);
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

	matrixDirty = true;
	vectorDirty = true;

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

	matrixDirty = true;
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
	matrixDirty = false;
}

void Transform::CalculateVectors()
{
	// Create a quaternion representing the current rotation
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Throw away the rotated vectors and recalculate them from the rotation quaternion
	// Rotate each of the default forward, right and up vectors with the rotation quaternion
	XMStoreFloat3(&forward, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotQuat));
	XMStoreFloat3(&right, XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotQuat));
	XMStoreFloat3(&up, XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotQuat));

	// Reset the dirty flag
	vectorDirty = false;
}

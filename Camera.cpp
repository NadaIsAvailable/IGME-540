#include <cmath>

#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(
	float aspectRatio, 
	DirectX::XMFLOAT3 pos, 
	DirectX::XMFLOAT3 rot, 
	float fov,
	float nearPlane, 
	float farPlane, 
	float movementSpeed, 
	float lookSpeed, 
	bool isPerspective) :
	aspectRatio(aspectRatio),
	fov(fov),
	nearPlane(nearPlane),
	farPlane(farPlane),
	movementSpeed(movementSpeed),
	lookSpeed(lookSpeed),
	isPerspective(isPerspective)
{
	// Create the transform and set the position and rotation
	transform = std::make_shared<Transform>();
	transform->SetPosition(pos);
	transform->SetRotation(rot);

	// Calculate the initial view and projection matrices
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

DirectX::XMFLOAT4X4 Camera::GetView()
{
	return view;
}

DirectX::XMFLOAT4X4 Camera::GetProjection()
{
	return projection;
}

std::shared_ptr<Transform> Camera::GetTransform()
{
	return transform;
}

float Camera::GetFov()
{
	return fov;
}

float Camera::GetNearPlane()
{
	return nearPlane;
}

float Camera::GetFarPlane()
{
	return farPlane;
}

float Camera::GetMovementSpeed()
{
	return movementSpeed;
}

float Camera::GetLookSpeed()
{
	return lookSpeed;
}

void Camera::SetFov(float fov)
{
	// Ignore the change is the new fov is invalid 
	// (<= 0 or >= 180 degrees)
	if (fov <= 0.0f || fov >= XM_PI) 
		return;

	// set fov and update projection matrix with new fov
	this->fov = fov;
	UpdateProjectionMatrix(aspectRatio);
}

void Camera::SetNearPlane(float nearPlane)
{
	// Ignore the change if the new near plane is invalid
	// (<= 0 or >= far plane)
	if (nearPlane <= 0.0f || nearPlane >= farPlane) 
		return;

	// set near plane and update projection matrix with new near plane
	this->nearPlane = nearPlane;
	UpdateProjectionMatrix(aspectRatio);
}

void Camera::SetFarPlane(float farPlane)
{
	// Ignore the change if the new far plane is invalid
	// (>= 1000 or <= near plane)
	if (farPlane >= 1000.0f || farPlane <= nearPlane) 
		return;

	// set far plane and update projection matrix with new far plane
	this->farPlane = farPlane;
	UpdateProjectionMatrix(aspectRatio);
}

void Camera::SetMovementSpeed(float movementSpeed)
{
	// Ignore the change if the new movement speed is negative
	if (movementSpeed < 0.0f) return;

	this->movementSpeed = movementSpeed;
}

void Camera::SetLookSpeed(float lookSpeed)
{
	// Ignore the change if the new look speed is negative
	if (lookSpeed < 0.0f) return;

	this->lookSpeed = lookSpeed;
}

void Camera::UpdateViewMatrix()
{
	// Grab the position and forward vector from the transform
	XMFLOAT3 pos = transform->GetPosition();
	XMFLOAT3 foward = transform->GetForward();

	// Create view matrix
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&pos),	
		XMLoadFloat3(&foward),
		XMVectorSet(0, 1, 0, 0)		// world up vector
	);

	// Store the view matrix
	XMStoreFloat4x4(&this->view, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
	if (isPerspective)
	{
		// Update stored aspect ratio
		this->aspectRatio = aspectRatio;

		// Create projection matrix
		XMMATRIX perspective = XMMatrixPerspectiveFovLH(
			fov,
			aspectRatio,
			nearPlane,
			farPlane
		);

		// Store the projection matrix
		XMStoreFloat4x4(&projection, perspective);
	}
	//else
		//XMMatrixOrthographicLH();
}

void Camera::Update(float dt)
{
	// Foward and backward
	if (Input::KeyDown('W')) transform->MoveAbsolute(0, 0, movementSpeed * dt);
	if (Input::KeyDown('S')) transform->MoveAbsolute(0, 0, -movementSpeed * dt);
	// Left and right
	if (Input::KeyDown('A')) transform->MoveAbsolute(-movementSpeed * dt, 0, 0);
	if (Input::KeyDown('D')) transform->MoveAbsolute(movementSpeed * dt, 0, 0);
	// Up and down (world)
	if (Input::KeyDown('Q')) transform->MoveRelative(0, movementSpeed * dt, 0);
	if (Input::KeyDown('E')) transform->MoveAbsolute(0, -movementSpeed * dt, 0);

	// Detect if left mouse btn is down
	if (Input::MouseLeftDown())
	{
		// Get mouse movement deltas
		float dx = Input::GetMouseXDelta() * lookSpeed;
		float dy = Input::GetMouseYDelta() * lookSpeed;

		// Apply rotation to transform
		transform->Rotate(dy, dx, 0);

		// Clamp the x rotation to prevent flipping the camera upside down
		XMFLOAT3 pyr = transform->GetPitchYawRoll();
		float maxXRot = XM_PIDIV2 - 0.001f;	// just a little less than 90 degrees
		if (std::abs(pyr.x) > XM_PIDIV2) {
			// clamp to ~(-)PI/2
			transform->SetRotation(pyr.x > 0 ? maxXRot : -maxXRot);
		}
	}

	UpdateViewMatrix();
}

#pragma once

#include <DirectXMath.h>
#include "Lights.h"

#define MAX_LIGHTS 128

struct VSConstantBuffer
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInvTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct PSConstantBuffer
{
	DirectX::XMFLOAT4 colorTint;

	float totalTime;
	DirectX::XMFLOAT3 intensities;

	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT2 offset;

	DirectX::XMFLOAT3 cameraPos;
	float padding1;

	DirectX::XMFLOAT3 ambientColor;
	float padding2;

	Light lights[MAX_LIGHTS];
	int lightCount;
};
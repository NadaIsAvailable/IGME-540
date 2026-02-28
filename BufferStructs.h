#pragma once

#include <DirectXMath.h>

struct VSConstantBuffer
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct PSConstantBuffer
{
	DirectX::XMFLOAT4 colorTint;
	float totalTime;
	DirectX::XMFLOAT3 intensities;
};
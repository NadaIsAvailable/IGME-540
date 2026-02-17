#pragma once

#include <DirectXMath.h>

struct VSConstantBuffer
{
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT4X4 world;
};
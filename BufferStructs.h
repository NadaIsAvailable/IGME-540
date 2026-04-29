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
	DirectX::XMFLOAT4X4 lightView;
	DirectX::XMFLOAT4X4 lightProjection;
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

	/*DirectX::XMFLOAT3 ambientColor;
	float padding2;*/

	Light lights[MAX_LIGHTS];
	int lightCount;
};

struct SkyVSConstantBuffer
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct ShadowOptions 
{
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;
	int shadowMapResolution;	// Ideally a power of 2 (like 1024)
	float lightProjectionSize;
};

struct PostProcessOptions
{
	bool postProcessEnabled;
	bool bloomEnabled;
	bool blurEnabled;
	int blurDistance;
	float bloomThreshold;
};
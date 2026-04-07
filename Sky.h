#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

#include "Mesh.h"
#include "BufferStructs.h"
#include "Camera.h"

class Sky
{
private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
	std::shared_ptr<Mesh> mesh;
	SkyVSConstantBuffer vsData;

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

public:
	Sky(
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler, 
		std::shared_ptr<Mesh> mesh,
		std::wstring vsFilePath,
		std::wstring psFilePath,
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back
	);

	void Draw(std::shared_ptr<Camera> camera);
};


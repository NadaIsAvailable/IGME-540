#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>

class Material
{
private:
	std::string name;

	// Properties applied to the texture in ps
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT2 offset;

	// Shaders
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;

	//Unordered maps holding SRVs and Sampler for a single material
	std::unordered_map<unsigned int, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<unsigned int, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;

public:
	Material(std::string name, DirectX::XMFLOAT4 colorTint, Microsoft::WRL::ComPtr<ID3D11PixelShader> ps, Microsoft::WRL::ComPtr<ID3D11VertexShader> vs);

	// Getters
	std::string GetName();
	DirectX::XMFLOAT4 GetColorTint();
	DirectX::XMFLOAT2 GetScale();
	DirectX::XMFLOAT2 GetOffset();
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader();
	Microsoft::WRL::ComPtr<ID3D11VertexShader> GetVertexShader();
	std::unordered_map<unsigned int, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& GetTextureSRVMap();

	// Setters
	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetScale(DirectX::XMFLOAT2 scale);
	void SetOffset(DirectX::XMFLOAT2 offset);
	void SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> ps);
	void SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> vs);

	void AddTextureSRV(unsigned int i, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(unsigned int i, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);

	// PS Helper
	void BindTexturesAndSamplers();
};


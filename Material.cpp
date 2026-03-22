#include "Material.h"
#include "Graphics.h"

Material::Material(
	std::string name, 
	DirectX::XMFLOAT4 colorTint, 
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps, 
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs) :
	name(name),
	colorTint(colorTint), 
	scale(DirectX::XMFLOAT2(1.0f, 1.0f)),
	offset(DirectX::XMFLOAT2(0.0f, 0.0f)),
	ps(ps), 
	vs(vs)
{
}

std::string Material::GetName()
{
	return name;
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

DirectX::XMFLOAT2 Material::GetScale()
{
	return scale;
}

DirectX::XMFLOAT2 Material::GetOffset()
{
	return offset;
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> Material::GetPixelShader()
{
    return ps;
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> Material::GetVertexShader()
{
    return vs;
}

std::unordered_map<unsigned int, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>& Material::GetTextureSRVMap()
{
	return textureSRVs;
}

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetScale(DirectX::XMFLOAT2 scale)
{
	this->scale = scale;
}

void Material::SetOffset(DirectX::XMFLOAT2 offset)
{
	this->offset = offset;
}

void Material::SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> ps)
{
	this->ps = ps;
}

void Material::SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> vs)
{
	this->vs = vs;
}

void Material::AddTextureSRV(unsigned int i, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	textureSRVs.insert({ i, srv });
}

void Material::AddSampler(unsigned int i, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	samplers.insert({ i, sampler });
}

void Material::BindTexturesAndSamplers()
{
	// Binding SRVs and Samplers in C++ (the first param is the index from the shader)

	for (auto& srv : textureSRVs)
		Graphics::Context->PSSetShaderResources(srv.first, 1, srv.second.GetAddressOf());

	for (auto& s : samplers)
		Graphics::Context->PSSetSamplers(s.first, 1, s.second.GetAddressOf());
}

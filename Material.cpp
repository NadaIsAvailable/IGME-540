#include "Material.h"

Material::Material(
	std::string name, 
	DirectX::XMFLOAT4 colorTint, 
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ps, 
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs) :
	name(name),
	colorTint(colorTint), 
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

Microsoft::WRL::ComPtr<ID3D11PixelShader> Material::GetPixelShader()
{
    return ps;
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> Material::GetVertexShader()
{
    return vs;
}

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
	this->colorTint = colorTint;
}

void Material::SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> ps)
{
	this->ps = ps;
}

void Material::SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> vs)
{
	this->vs = vs;
}

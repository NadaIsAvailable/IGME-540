#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Mesh.h"
#include "BufferStructs.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Material.h"
#include "Lights.h"
#include "Sky.h"

class Game
{
public:
	// Basic OOP setup
	Game();
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:
	// Used for UI purposes ---------------------------------------------------
	// UI components test
	float backgroundColor[4];
	bool showDemoWindow;
	int number;
	float testArrayPtr[2];
	char textInput[256];

	// Rotation controls
	bool rotateX;
	bool rotateY;
	bool rotateZ;
	// ------------------------------------------------------------------------

	// Constant Buffers
	VSConstantBuffer vsData{};
	PSConstantBuffer psData{};

	// Sky
	std::shared_ptr<Sky> sky;

	// Lightings
	// DirectX::XMFLOAT3 ambientColor;
	std::vector<Light> lights;

	// Meshes
	std::vector<std::shared_ptr<Mesh>> meshes;

	// Game entities
	std::vector<GameEntity> entities;

	// Cameras
	std::vector<std::shared_ptr<Camera>> cameras;
	int activeCamera;

	// Shadow mapping
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowVS;
	ShadowOptions shadowOptions;

	// Resources that are shared among all post processes
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> ppVS;
	// Resources that are tied to a particular post process
	Microsoft::WRL::ComPtr<ID3D11PixelShader> ppPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppRTV; // For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppSRV; // For sampling
	PostProcessOptions ppOptions;

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadTexture(std::wstring path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv);
	void CreateEntities();
	void SetUpInputLayoutAndGraphics();
	void UpdateImGui(float deltaTime);
	void BuildUI();
	void CreateShadowMapResources();
	void CreateShadowMap();
	void CreatePPResources();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};


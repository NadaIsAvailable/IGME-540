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
	// Used for UI purposes (with default values)
	// Assignment 02 - ImGui
	float backgroundColor[4];
	bool showDemoWindow;
	int number;
	float testArrayPtr[2];
	char textInput[256];

	// Constant Buffers
	VSConstantBuffer vsData{};
	PSConstantBuffer psData{};

	// Mesh class testing
	std::vector<std::shared_ptr<Mesh>> meshes;

	// GameEntity class testing
	std::vector<GameEntity> entities;

	// Camera class testing
	std::vector<std::shared_ptr<Camera>> cameras;
	int activeCamera;

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void CreateEntities();
	void SetUpInputLayoutAndGraphics();
	void UpdateImGui(float deltaTime);
	void BuildUI();

	Microsoft::WRL::ComPtr<ID3D11PixelShader> LoadPixelShader(std::wstring filePath);
	Microsoft::WRL::ComPtr<ID3D11VertexShader> LoadVertexShader(std::wstring filePath);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	// Constant buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> psConstantBuffer;
};


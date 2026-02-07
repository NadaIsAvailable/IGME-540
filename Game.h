#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Mesh.h"
#include "BufferStructs.h"

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
	float backgroundColor[4] { 0.4f, 0.6f, 0.75f, 0.0f };
	bool showDemoWindow = false;
	int number = 0;
	float testArrayPtr[2] { 0.5f, 0.5f };
	char textInput[256]{ "edit this text" };
	// Assignment 04 - Constant Buffer
	VSConstantBuffer vsData{};

	// Mesh class testing
	std::vector<std::shared_ptr<Mesh>> meshes;

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders();
	void CreateGeometry();

	void UpdateImGui(float deltaTime);
	void BuildUI();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	// Constant buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer;
};


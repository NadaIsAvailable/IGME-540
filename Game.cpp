#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"

#include <string>
#include <DirectXMath.h>

// Needed for loading textures
#include <WICTextureLoader.h>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// This code assumes files are in "ImGui" subfolder!
// Adjust as necessary for your own folder structure and project setup
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game() :
	backgroundColor(0.4f, 0.6f, 0.75f, 0.0f),
	showDemoWindow(false),
	number(0),
	testArrayPtr(0.5f, 0.5f),
	textInput("edit this text"),
	rotateX(false),
	rotateY(false),
	rotateZ(false)
	//ambientColor(0.1f, 0.1f, 0.25f)
{
	// Set ups
	CreateEntities();
	SetUpInputLayoutAndGraphics();

	// Create Cameras
	{
		// Create a few cameras and store in vector
		cameras.push_back(std::make_shared<Camera>(
			Window::AspectRatio(),
			XMFLOAT3(7.0f, 6.0f, -10.0f),
			XMFLOAT3(0.0f, 0.0f, 0.0f),
			10.0f)
		);

		cameras.push_back(std::make_shared<Camera>(
			Window::AspectRatio(),
			XMFLOAT3(0.5f, 0.5f, -2.0f))
		);
		cameras.push_back(std::make_shared<Camera>(
			Window::AspectRatio(),
			XMFLOAT3(-0.5f, -0.5f, -3.0f),
			XMFLOAT3(0.0f, XM_PIDIV4, 0.0f),
			XM_PI)
		);

		// set first camera as active camera
		activeCamera = 0;
	}

	// ImGui set ups
	{
		// Initialize ImGui itself & platform/renderer backends
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(Window::Handle());
		ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
		// Pick a style (uncomment one of these 3)
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		//ImGui::StyleColorsClassic();
	}

	// Struct initial data
	{
		vsData = {};
		// Set some initial data for the vs constant buffer
		DirectX::XMStoreFloat4x4(&vsData.world, XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&vsData.worldInvTranspose, XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&vsData.view, XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&vsData.projection, XMMatrixIdentity());

		psData = {};
		// Set some initial data for the ps constant buffer
		psData.intensities = XMFLOAT3(1.0f, 1.0f, 1.0f);
		psData.scale = XMFLOAT2(1.0f, 1.0f);
		psData.offset = XMFLOAT2(0.0f, 0.0f);
		psData.cameraPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		//psData.ambientColor = ambientColor;

		shadowOptions = {};
		// Set some initial data for the shadow mapping options struct
		shadowOptions.shadowMapResolution = 1024;
		shadowOptions.lightProjectionSize = 15.0f;

		ppOptions = {};
		// Set some initial data for the post process options struct
		ppOptions.postProcessEnabled = true;
		ppOptions.blurDistance = 5;
	}

	// Create shadow map resources (after shadow options filled in)
	CreateShadowMapResources();
}

// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// update all camera's projection matrices
	for (auto& camera : cameras)
	{
		if (camera) camera->UpdateProjectionMatrix(Window::AspectRatio());
	}

	CreatePPResources();
}

// --------------------------------------------------------
// Load texture helper
// --------------------------------------------------------
void Game::LoadTexture(std::wstring path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv)
{
	CreateWICTextureFromFile(
		Graphics::Device.Get(),
		Graphics::Context.Get(),
		FixPath(path).c_str(),
		0,
		srv->GetAddressOf());
}

// --------------------------------------------------------
// Create game entities - load shader + create materials + load meshes -> entities
// --------------------------------------------------------
void Game::CreateEntities()
{
	// Load shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader> basicVS = Graphics::LoadVertexShader(L"VertexShader.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> basicPS = Graphics::LoadPixelShader(L"PixelShader.cso");
	// Microsoft::WRL::ComPtr<ID3D11PixelShader> uvPS = LoadPixelShader(L"DebugUVsPS.cso");
	// Microsoft::WRL::ComPtr<ID3D11PixelShader> normalPS = LoadPixelShader(L"DebugNormalsPS.cso");
	// Microsoft::WRL::ComPtr<ID3D11PixelShader> customPS = LoadPixelShader(L"CustomPS.cso");
	// Microsoft::WRL::ComPtr<ID3D11PixelShader> combinePS = LoadPixelShader(L"CombineTexPS.cso");

	// Sampler
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;

	// Sampler description
	D3D11_SAMPLER_DESC sd = {};
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.MaxAnisotropy = 16;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the sampler
	Graphics::Device->CreateSamplerState(&sd, sampler.GetAddressOf());

	// Texture shader resource views
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedMetal;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughness;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetal;

	// Load textures
	LoadTexture(L"../../Assets/Textures/bronze_albedo.png", &bronzeAlbedo);
	LoadTexture(L"../../Assets/Textures/bronze_normals.png", &bronzeNormal);
	LoadTexture(L"../../Assets/Textures/bronze_roughness.png", &bronzeRoughness);
	LoadTexture(L"../../Assets/Textures/bronze_metal.png", &bronzeMetal);

	LoadTexture(L"../../Assets/Textures/cobblestone_albedo.png", &cobblestoneAlbedo);
	LoadTexture(L"../../Assets/Textures/cobblestone_normals.png", &cobblestoneNormal);
	LoadTexture(L"../../Assets/Textures/cobblestone_roughness.png", &cobblestoneRoughness);
	LoadTexture(L"../../Assets/Textures/cobblestone_metal.png", &cobblestoneMetal);

	LoadTexture(L"../../Assets/Textures/floor_albedo.png", &floorAlbedo);
	LoadTexture(L"../../Assets/Textures/floor_normals.png", &floorNormal);
	LoadTexture(L"../../Assets/Textures/floor_roughness.png", &floorRoughness);
	LoadTexture(L"../../Assets/Textures/floor_metal.png", &floorMetal);

	LoadTexture(L"../../Assets/Textures/paint_albedo.png", &paintAlbedo);
	LoadTexture(L"../../Assets/Textures/paint_normals.png", &paintNormal);
	LoadTexture(L"../../Assets/Textures/paint_roughness.png", &paintRoughness);
	LoadTexture(L"../../Assets/Textures/paint_metal.png", &paintMetal);

	LoadTexture(L"../../Assets/Textures/rough_albedo.png", &roughAlbedo);
	LoadTexture(L"../../Assets/Textures/rough_normals.png", &roughNormal);
	LoadTexture(L"../../Assets/Textures/rough_roughness.png", &roughRoughness);
	LoadTexture(L"../../Assets/Textures/rough_metal.png", &roughMetal);

	LoadTexture(L"../../Assets/Textures/scratched_albedo.png", &scratchedAlbedo);
	LoadTexture(L"../../Assets/Textures/scratched_normals.png", &scratchedNormal);
	LoadTexture(L"../../Assets/Textures/scratched_roughness.png", &scratchedRoughness);
	LoadTexture(L"../../Assets/Textures/scratched_metal.png", &scratchedMetal);

	LoadTexture(L"../../Assets/Textures/wood_albedo.png", &woodAlbedo);
	LoadTexture(L"../../Assets/Textures/wood_normals.png", &woodNormal);
	LoadTexture(L"../../Assets/Textures/wood_roughness.png", &woodRoughness);
	LoadTexture(L"../../Assets/Textures/wood_metal.png", &woodMetal);

	// Make materials
	// Set textures and sampler for each material
	std::shared_ptr<Material> bronze = std::make_shared<Material>("Bronze", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	bronze->AddTextureSRV(0, bronzeAlbedo);
	bronze->AddTextureSRV(1, bronzeNormal);
	bronze->AddTextureSRV(2, bronzeRoughness);
	bronze->AddTextureSRV(3, bronzeMetal);
	bronze->AddSampler(0, sampler);

	std::shared_ptr<Material> cobblestone = std::make_shared<Material>("Cobblestone", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	cobblestone->AddTextureSRV(0, cobblestoneAlbedo);
	cobblestone->AddTextureSRV(1, cobblestoneNormal);
	cobblestone->AddTextureSRV(2, cobblestoneRoughness);
	cobblestone->AddTextureSRV(3, cobblestoneMetal);
	cobblestone->AddSampler(0, sampler);

	std::shared_ptr<Material> floor = std::make_shared<Material>("Floor", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	floor->AddTextureSRV(0, floorAlbedo);
	floor->AddTextureSRV(1, floorNormal);
	floor->AddTextureSRV(2, floorRoughness);
	floor->AddTextureSRV(3, floorMetal);
	floor->AddSampler(0, sampler);

	std::shared_ptr<Material> paint = std::make_shared<Material>("Paint", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	paint->AddTextureSRV(0, paintAlbedo);
	paint->AddTextureSRV(1, paintNormal);
	paint->AddTextureSRV(2, paintRoughness);
	paint->AddTextureSRV(3, paintMetal);
	paint->AddSampler(0, sampler);

	std::shared_ptr<Material> rough = std::make_shared<Material>("Rough", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	rough->AddTextureSRV(0, roughAlbedo);
	rough->AddTextureSRV(1, roughNormal);
	rough->AddTextureSRV(2, roughRoughness);
	rough->AddTextureSRV(3, roughMetal);
	rough->AddSampler(0, sampler);

	std::shared_ptr<Material> scratched = std::make_shared<Material>("Scratched", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	scratched->AddTextureSRV(0, scratchedAlbedo);
	scratched->AddTextureSRV(1, scratchedNormal);
	scratched->AddTextureSRV(2, scratchedRoughness);
	scratched->AddTextureSRV(3, scratchedMetal);
	scratched->AddSampler(0, sampler);

	std::shared_ptr<Material> wood = std::make_shared<Material>("Wood", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), basicPS, basicVS);
	wood->AddTextureSRV(0, woodAlbedo);
	wood->AddTextureSRV(1, woodNormal);
	wood->AddTextureSRV(2, woodRoughness);
	wood->AddTextureSRV(3, woodMetal);
	wood->AddSampler(0, sampler);

	// Make meshes from the .obj files
	meshes.push_back(std::make_shared<Mesh>("Cube", FixPath("../../Assets/Meshes/cube.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Cylinder" ,FixPath("../../Assets/Meshes/cylinder.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Helix" ,FixPath("../../Assets/Meshes/helix.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Sphere" ,FixPath("../../Assets/Meshes/sphere.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Torus" ,FixPath("../../Assets/Meshes/torus.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Quad" ,FixPath("../../Assets/Meshes/quad.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>("Quad (Double-Sided)" ,FixPath("../../Assets/Meshes/quad_double_sided.obj").c_str()));

	// Make entities from the meshes and materials
	entities.push_back(GameEntity(meshes[3], bronze));
	entities.push_back(GameEntity(meshes[0], cobblestone));
	entities.push_back(GameEntity(meshes[1], floor));
	entities.push_back(GameEntity(meshes[2], paint));
	entities.push_back(GameEntity(meshes[3], rough));
	entities.push_back(GameEntity(meshes[3], scratched));
	entities.push_back(GameEntity(meshes[3], wood));
	entities.push_back(GameEntity(meshes[6], wood));	// floor for shadows

	// Spread out the entities
	float x = -5.0f;
	float y = 4.0f;
	int i = 0;
	int meshPerRow = 7;
	int rows = (int)std::ceil((float)meshes.size() / meshPerRow);
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < meshPerRow; c++)
		{
			if (i >= meshes.size()) break;

			entities[i].GetTransform()->SetPosition(x, y, 0.0f);

			x += 4.0f;
			i++;
		}
		x = -5.0f;
		y -= 4.0f;
	}

	entities[0].GetTransform()->SetPosition(-5.0f, 8.0f, -4.0f);

	// Set up the floor entity to be larger and under all other entities
	entities[7].GetTransform()->SetPosition(5.0f, -2.0f, 0.0f);
	entities[7].GetTransform()->SetScale(30.0f, 1.0f, 20.0f);

	// Lights
	Light dirLight1 = {};	// shadow casting light
	dirLight1.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight1.Direction = XMFLOAT3(0.0f, -0.707f, 0.707f);
	dirLight1.Color = XMFLOAT3(1.f, 1.0f, 1.0f);
	dirLight1.Intensity = 0.5f;
	lights.push_back(dirLight1);

	Light dirLight2 = {};
	dirLight2.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight2.Direction = XMFLOAT3(0.0f, 1.0f, 0.0f);
	dirLight2.Color = XMFLOAT3(0.f, 1.0f, 0.0f);
	dirLight2.Intensity = 0.5f;
	lights.push_back(dirLight2);
	
	Light dirLight3 = {};
	dirLight3.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight3.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	dirLight3.Color = XMFLOAT3(0.f, 0.0f, 1.0f);
	dirLight3.Intensity = 0.5f;
	lights.push_back(dirLight3);
	
	Light dirLight4 = {};
	dirLight4.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight4.Direction = XMFLOAT3(1.0f, 1.0f, 0.0f);
	dirLight4.Color = XMFLOAT3(1.f, 1.0f, 0.0f);
	dirLight4.Intensity = 0.5f;
	lights.push_back(dirLight4);
	
	Light dirLight5 = {};
	dirLight5.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight5.Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	dirLight5.Color = XMFLOAT3(0.f, 1.0f, 1.0f);
	dirLight5.Intensity = 0.5f;
	lights.push_back(dirLight5);
	
	Light pointLight1 = {};
	pointLight1.Type = LIGHT_TYPE_POINT;
	pointLight1.Position = XMFLOAT3(3.0f, 4.0f, 0.0f);
	pointLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	pointLight1.Range = 8.0f;
	pointLight1.Intensity = 1.0f;
	lights.push_back(pointLight1);
	
	Light spotLight1 = {};
	spotLight1.Type = LIGHT_TYPE_SPOT;
	spotLight1.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	spotLight1.Position = XMFLOAT3(15.0f, 10.0f, 0.0f);
	spotLight1.Color = XMFLOAT3(1.f, 0.0f, 1.0f);
	spotLight1.Range = 10.0f;
	spotLight1.SpotInnerAngle = XM_PI / 64.0f;
	spotLight1.SpotOuterAngle = XM_PI / 32.0f;
	spotLight1.Intensity = 2.0f;
	lights.push_back(spotLight1);

	// Sky 
	sky = std::make_shared<Sky>(
		sampler,
		meshes[0], // cube
		L"SkyVS.cso",
		L"SkyPS.cso",
		FixPath(L"../../Assets/Textures/sky/right.png").c_str(),
		FixPath(L"../../Assets/Textures/sky/left.png").c_str(),
		FixPath(L"../../Assets/Textures/sky/up.png").c_str(),
		FixPath(L"../../Assets/Textures/sky/down.png").c_str(),
		FixPath(L"../../Assets/Textures/sky/front.png").c_str(),
		FixPath(L"../../Assets/Textures/sky/back.png").c_str()
	);

	// Create post process resources
	// Set up vertex shader and pixel shader
	ppVS = Graphics::LoadVertexShader(L"FullscreenVS.cso");
	ppPS = Graphics::LoadPixelShader(L"PostProcessPS.cso");

	CreatePPResources();

	// Sampler state for post processing
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Graphics::Device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());
}

// --------------------------------------------------------
// Set ups for GPU and D3D stuffs
// --------------------------------------------------------
void Game::SetUpInputLayoutAndGraphics()
{
	// Create an input layout
	{
		ID3DBlob* vertexShaderBlob;
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create an input layout 
		//  - This describes the layout of data sent to a vertex shader
		//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
		//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
		//  - Luckily, we already have that loaded (the vertex shader blob above)
		D3D11_INPUT_ELEMENT_DESC inputElements[4] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a UV coordinate, which is 2 float values
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;					// 2x 32-bit floats
		inputElements[1].SemanticName = "TEXCOORD";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Set up the third element - a normal, which is 3 float values
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// 3x 32-bit floats
		inputElements[2].SemanticName = "NORMAL";							// Match our vertex shader input!
		inputElements[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Set up the fourth element - a tangent, which is 3 more float values
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			4,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());
	}
}

// --------------------------------------------------------
// UI helper methods
// --------------------------------------------------------
void Game::UpdateImGui(float deltaTime)
{
	// Put this all in a helper method that is called from Game::Update()
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);

	// Show the demo window
	if (showDemoWindow) ImGui::ShowDemoWindow();
}

void Game::BuildUI()
{
	// Begin building custom ui window
	ImGui::Begin("Inspector");

	// Assignment 02 - ImGui
	if (ImGui::CollapsingHeader("App Details"))
	{
		// Frame rate display
		ImGui::Text("Frame Rate: %f fps", ImGui::GetIO().Framerate);

		// Window size display
		ImGui::Text("Window Client Size: %ix%i", Window::Width(), Window::Height());

		// Background color editor
		ImGui::ColorEdit4("Background Color Editor", backgroundColor);

		// Button to toggle on and off the demo window
		if (ImGui::Button("Toggle Demo Window"))
			showDemoWindow = !showDemoWindow;

		// UI components test
		ImGui::SliderInt("Choose a number", &number, 0, 100);
		ImGui::DragFloat2("2-component editor", testArrayPtr);
		ImGui::InputText("Input Text Label", textInput, 256);
		ImGui::Text("Current text: %s", textInput);
	}

	// Assignment 03 - Mesh class
	/*
	if (ImGui::CollapsingHeader("Meshes"))
	{
		for (auto& mesh : meshes) 
		{
			// header for each mesh
			std::string header = "Mesh: " + mesh->GetName();
			if (ImGui::CollapsingHeader(header.c_str()))
			{
				// mesh info
				ImGui::Text("Triangles: %i", mesh->GetIndexCount() / 3);
				ImGui::Text("Vertices: %i", mesh->GetVertexCount());
				ImGui::Text("Indices: %i", mesh->GetIndexCount());
			}
		}
	}
	*/

	// Assignment 04 - Constant Buffer
	/*
	if (ImGui::CollapsingHeader("Constant Buffer")) 
	{
		// Color tint editor
		ImGui::ColorEdit4("Color Tint Editor", &vsData.colorTint.x);

		// Offset editor
		ImGui::DragFloat3("Offset Editor", &vsData.offset.x, 0.01f);
	}
	*/

	// Game Entities
	if (ImGui::CollapsingHeader("Scene Entities"))
	{
		ImGui::Checkbox("Rotate about X", &rotateX);
		ImGui::Checkbox("Rotate about Y", &rotateY);
		ImGui::Checkbox("Rotate about Z", &rotateZ);

		for (uint i = 0; i < entities.size(); i++)
		{
			// header for each entity
			std::string header = "Entity " + std::to_string(i) +
				" (" + entities[i].GetMesh()->GetName() + ')';
			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::PushID(i);

				// Get material of this entity
				std::shared_ptr<Material> material = entities[i].GetMaterial();

				// Display material name
				std::string matName = "Material Name: " + material->GetName();
				ImGui::Text(matName.c_str());

				// Display srvs in a table (4 in a row)
				if (ImGui::BeginTable("Textures for Entity " + i, 4))
				{
					int idx = 0;
					for (auto& tex : material->GetTextureSRVMap())
					{
						if (idx % 4 == 0)
							ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(idx % 4);

						ImGui::Text("Texture %i", tex.first + 1);
						ImGui::Image(tex.second.Get(), ImVec2(128, 128));

						idx++;
					}
					ImGui::EndTable();
				}

				// Material control options -----------------------------------
				XMFLOAT4 colorTint = material->GetColorTint();
				if(ImGui::SliderFloat4("Color Tint", &colorTint.x, 0.0f, 1.0f)) 
					material->SetColorTint(colorTint);

				XMFLOAT2 matScale = material->GetScale();
				if(ImGui::SliderFloat2("Material Scale", &matScale.x, 0.0f, 5.0f)) 
					material->SetScale(matScale);

				XMFLOAT2 matOffset = material->GetOffset();
				if (ImGui::SliderFloat2("Material Offset", &matOffset.x, -5.0f, 5.0f)) 
					material->SetOffset(matOffset);
				// ------------------------------------------------------------
				
				// Transform control options ----------------------------------
				XMFLOAT3 pos = entities[i].GetTransform()->GetPosition();
				if(ImGui::DragFloat3("Position", &pos.x, 0.01f))
					entities[i].GetTransform()->SetPosition(pos);

				XMFLOAT3 rot = entities[i].GetTransform()->GetPitchYawRoll();
				if(ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f))
					entities[i].GetTransform()->SetRotation(rot);

				XMFLOAT3 scale = entities[i].GetTransform()->GetScale();
				if(ImGui::DragFloat3("Scale", &scale.x, 0.01f))
					entities[i].GetTransform()->SetScale(scale);
				// ------------------------------------------------------------

				ImGui::PopID();
			}
		}
	}

	// Camera
	if (ImGui::CollapsingHeader("Cameras"))
	{
		// Select active camera------------------------------------------------
		// Hardcoding the names...
		const char* camNames[] = { "Camera 0", "Camera 1", "Camera 2" };

		// Set up the dropdown
		ImGui::Combo("Active Camera", &activeCamera, camNames, IM_ARRAYSIZE(camNames));

		// Camera transform toggles -------------------------------------------
		for (uint i = 0; i < cameras.size(); i++)
		{
			// header for each camera
			std::string header = "Camera " + std::to_string(i);
			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::PushID(i);

				// Position
				XMFLOAT3 pos = cameras[i]->GetTransform()->GetPosition();
				if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
					cameras[i]->GetTransform()->SetPosition(pos);

				// Rotation
				XMFLOAT3 rot = cameras[i]->GetTransform()->GetPitchYawRoll();
				if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f))
					cameras[i]->GetTransform()->SetRotation(rot);

				// FOV
				float fov = cameras[i]->GetFov();
				if (ImGui::DragFloat("Field of View (Radians)", &fov, 0.01f, 0.01f, XM_PI))
					cameras[i]->SetFov(fov);

				// Near plane
				float nearPlane = cameras[i]->GetNearPlane();
				if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, cameras[i]->GetFarPlane()))
					cameras[i]->SetNearPlane(nearPlane);

				// Far plane
				float farPlane = cameras[i]->GetFarPlane();
				if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, cameras[i]->GetNearPlane(), 1000.0f))
					cameras[i]->SetFarPlane(farPlane);

				// Movement speed
				float movementSpeed = cameras[i]->GetMovementSpeed();
				if (ImGui::DragFloat("Movement Speed", &movementSpeed, 0.01f, 0.01f, 20.0f))
					cameras[i]->SetMovementSpeed(movementSpeed);

				// Look speed
				float lookSpeed = cameras[i]->GetLookSpeed();
				if (ImGui::DragFloat("Look Speed", &lookSpeed, 0.001f, 0.001f, 0.05f))
					cameras[i]->SetLookSpeed(lookSpeed);

				ImGui::PopID();
			}
		}
	}

	// Custom Shader Controls
	/*
	if (ImGui::CollapsingHeader("Custom Shader"))
	{
		XMFLOAT3 intensity = psData.intensities;
		if (ImGui::SliderFloat3("RGB Intensities", &intensity.x, 0.0f, 1.0f))
			DirectX::XMStoreFloat3(&psData.intensities, XMLoadFloat3(&intensity));
	}
	*/

	// Lighting
	if (ImGui::CollapsingHeader("Lights"))
	{
		// ImGui::ColorEdit3("Ambient Color", &ambientColor.x);

		for (uint i = 0; i < lights.size(); i++)
		{
			Light& light = lights[i];

			// header for each light
			std::string type = "";

			if (light.Type == LIGHT_TYPE_DIRECTIONAL) type = "Directional";
			else if (light.Type == LIGHT_TYPE_POINT) type = "Point";
			else if (light.Type == LIGHT_TYPE_SPOT) type = "Spot";

			std::string header = "Light " + std::to_string(i) + " (" + type + ')';

			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::PushID(i);

				// Light Type
				if (ImGui::RadioButton("Directional", light.Type == LIGHT_TYPE_DIRECTIONAL))
					light.Type = LIGHT_TYPE_DIRECTIONAL;

				if (ImGui::RadioButton("Point", light.Type == LIGHT_TYPE_POINT))
					light.Type = LIGHT_TYPE_POINT;

				if (ImGui::RadioButton("Spot", light.Type == LIGHT_TYPE_SPOT))
					light.Type = LIGHT_TYPE_SPOT;

				// Light intensity
				ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 10.0f);

				// Light color
				ImGui::ColorEdit4("Color", &light.Color.x);

				// Directional and spot lights
				if (light.Type == LIGHT_TYPE_DIRECTIONAL || light.Type == LIGHT_TYPE_SPOT)
				{
					// Direction
					if (ImGui::DragFloat3("Direction", &light.Direction.x, 0.1f)) 
						XMStoreFloat3(&light.Direction, XMVector3Normalize(XMLoadFloat3(&light.Direction)));
				}
				 
				// Point and spot lights
				if (light.Type == LIGHT_TYPE_POINT || light.Type == LIGHT_TYPE_SPOT) 
				{
					// Position
					ImGui::DragFloat3("Position", &light.Position.x, 0.1f);

					// Range
					ImGui::SliderFloat("Range", &light.Range, 0.0f, 20.0f);
				}

				// Spot lights only
				if (light.Type == LIGHT_TYPE_SPOT)
				{
					// Inner and outer angles
					ImGui::SliderFloat("Spot Inner Angle", &light.SpotInnerAngle, 0.0f, light.SpotOuterAngle - 0.001f);
					ImGui::SliderFloat("Spot Outer Angle", &light.SpotOuterAngle, light.SpotInnerAngle + 0.001f, XM_PI);
				}

				ImGui::PopID();
			}
		}
	}

	// Shadow Mapping
	if (ImGui::CollapsingHeader("Shadow Mapping"))
	{
		// ImGui::SliderInt("Shadow Map Resolution", &shadowOptions.shadowMapResolution, 256, 4096);
		// ImGui::SliderFloat("Light Projection Size", &shadowOptions.lightProjectionSize, 1.0f, 50.0f);
		ImGui::Text("Shadow Map");
		ImGui::Image(shadowSRV.Get(), ImVec2(256, 256));
	}

	// Post Processing
	if (ImGui::CollapsingHeader("Post Processing"))
	{
		ImGui::Checkbox("Enable Post Processing", &ppOptions.postProcessEnabled);
		ImGui::SliderInt("Blur Distance", &ppOptions.blurDistance, 0, 10);
		ImGui::Text("Pre-Process");
		ImGui::Image(ppSRV.Get(), ImVec2(Window::Width() / 4, Window::Height() / 4));
	}

	// End custom ui window
	ImGui::End();
}

// --------------------------------------------------------
// Create any resources needed for shadow mapping here
// --------------------------------------------------------
void Game::CreateShadowMapResources()
{
	shadowDSV.Reset();
	shadowSRV.Reset();
	shadowRasterizer.Reset();
	shadowSampler.Reset();

	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowOptions.shadowMapResolution;
	shadowDesc.Height = shadowOptions.shadowMapResolution;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	Graphics::Device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Graphics::Device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	// Create a RS for the shadow map
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = false; // Keep out-of-frustum objects!
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	Graphics::Device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	// Create a sampler for the shadow map
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	Graphics::Device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	// The first light is the shadow casting directional light
	XMVECTOR lightDirection = XMLoadFloat3(&lights[0].Direction);
	XMStoreFloat4x4(
		&shadowOptions.lightViewMatrix, 
		XMMatrixLookToLH(
			-lightDirection * 20,		// Position: "Backing up" 20 units from origin
			lightDirection,				// Direction: light's direction
			XMVectorSet(0, 1, 0, 0))	// Up: World up vector (Y axis)
	);

	XMStoreFloat4x4(
		&shadowOptions.lightProjectionMatrix,
		XMMatrixOrthographicLH(
			shadowOptions.lightProjectionSize,
			shadowOptions.lightProjectionSize,
			1.0f,
			100.0f)
	);

	// Set up shadow vertex shader
	shadowVS = Graphics::LoadVertexShader(L"ShadowVS.cso");
}

// --------------------------------------------------------
// Create the shadow map each frame in this method (called from Game::Draw())
// --------------------------------------------------------
void Game::CreateShadowMap()
{
	// Shadow map settings
	// Reset all depth values to 1.0
	Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Bind to the shadow map render target instead of the back buffer
	ID3D11RenderTargetView* nullRTV{};
	Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

	// Set the rasterizer state to add depth bias to get rid of shadow acne
	Graphics::Context->RSSetState(shadowRasterizer.Get());

	// Set shadow VS and deactivate PS
	Graphics::Context->VSSetShader(shadowVS.Get(), 0, 0);
	Graphics::Context->PSSetShader(0, 0, 0);

	// Match the viewport size to the shadow map resolution
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowOptions.shadowMapResolution;
	viewport.Height = (float)shadowOptions.shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	Graphics::Context->RSSetViewports(1, &viewport);

	struct ShadowVSData
	{
		XMFLOAT4X4 world;
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;
	};

	ShadowVSData shadowVSData = {};
	shadowVSData.view = shadowOptions.lightViewMatrix;
	shadowVSData.proj = shadowOptions.lightProjectionMatrix;

	// Loop and draw all entities
	for (auto& e : entities)
	{
		shadowVSData.world = e.GetTransform()->GetWorldMatrix();
		Graphics::FillAndBindNextConstantBuffer(
			&shadowVSData,
			sizeof(ShadowVSData),
			D3D11_VERTEX_SHADER,
			0);
		e.Draw();
	}

	// Change settings back to normal for regular drawing in Game::Draw()
	if (ppOptions.postProcessEnabled)
		Graphics::Context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), Graphics::DepthBufferDSV.Get());
	else 
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	
	viewport.Width = (float)Window::Width();
	viewport.Height = (float)Window::Height();
	Graphics::Context->RSSetViewports(1, &viewport);
	Graphics::Context->RSSetState(0);
}

// --------------------------------------------------------
// Create any resources needed for post processing here
// --------------------------------------------------------
void Game::CreatePPResources()
{
	ppSRV.Reset();
	ppRTV.Reset();

	// Describe the texture we're creating
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = Window::Width();
	textureDesc.Height = Window::Height();
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture;
	Graphics::Device->CreateTexture2D(&textureDesc, 0, ppTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	Graphics::Device->CreateRenderTargetView(
		ppTexture.Get(),
		&rtvDesc,
		ppRTV.ReleaseAndGetAddressOf());
	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	Graphics::Device->CreateShaderResourceView(
		ppTexture.Get(),
		0,
		ppSRV.ReleaseAndGetAddressOf());
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();

	UpdateImGui(deltaTime);

	BuildUI();

	for (auto& entity : entities)
	{
		// Calcualte the rotation
		XMFLOAT3 rot(0.0f, 0.0f, 0.0f);
		float rotAmt = (float)sin(deltaTime * 0.5f);

		if (rotateX) rot.x = rotAmt;
		if (rotateY) rot.y = rotAmt;
		if (rotateZ) rot.z = rotAmt;

		// Rotate each entity a little
		entity.GetTransform()->Rotate(rot);
	}

	entities[0].GetTransform()->MoveAbsolute(XMFLOAT3((float)sin(totalTime) * 0.01f, 0.0f, 0.0f));

	cameras[activeCamera]->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(),	backgroundColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Pre-render before post-processing
		if (ppOptions.postProcessEnabled)
		{
			// Clear post process target too
			const float rtClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			Graphics::Context->ClearRenderTargetView(ppRTV.Get(), rtClearColor);

			// Change the render target to render directly into our post-process texture
			Graphics::Context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), Graphics::DepthBufferDSV.Get());
		}
	}

	CreateShadowMap();

	// Bind shadow resources to PS
	Graphics::Context->PSSetShaderResources(4, 1, shadowSRV.GetAddressOf());
	Graphics::Context->PSSetSamplers(1, 1, shadowSampler.GetAddressOf());

	// Update the constant buffer data (same for all entities)
	vsData.view = cameras[activeCamera]->GetView();
	vsData.projection = cameras[activeCamera]->GetProjection();
	vsData.lightView = shadowOptions.lightViewMatrix;
	vsData.lightProjection = shadowOptions.lightProjectionMatrix;

	psData.totalTime = totalTime;
	psData.cameraPos = cameras[activeCamera]->GetTransform()->GetPosition();
	// psData.ambientColor = ambientColor;
	memcpy(&psData.lights, &lights[0], sizeof(Light) * (int)lights.size());
	psData.lightCount = (int)lights.size();

	// Draw all entities
	for (auto& entity : entities)
	{
		std::shared_ptr<Material> material = entity.GetMaterial();

		// Pass material's scale and offset to ps
		psData.scale = material->GetScale();
		psData.offset = material->GetOffset();
		
		// Bind textures and samplers
		material->BindTexturesAndSamplers();

		// Activate the shaders for this material
		Graphics::Context->VSSetShader(material->GetVertexShader().Get(), 0, 0);
		Graphics::Context->PSSetShader(material->GetPixelShader().Get(), 0, 0);

		// Update constant buffers (entity specific)
		// Get matrices for this entity and store it in the constant buffer data
		vsData.world = entity.GetTransform()->GetWorldMatrix();
		vsData.worldInvTranspose = entity.GetTransform()->GetWorldInverseTransposeMatrix();
		// Get the color tint of this entity's material
		psData.colorTint = material->GetColorTint();

		// Fill and bind Vertex Shader Constant Buffer
		Graphics::FillAndBindNextConstantBuffer(
			&vsData,
			sizeof(VSConstantBuffer),
			D3D11_VERTEX_SHADER,
			0);
		
		// Fill and bind Pixel Shader Constant Buffer
		Graphics::FillAndBindNextConstantBuffer(
			&psData,
			sizeof(PSConstantBuffer),
			D3D11_PIXEL_SHADER,
			0);

		// Draw the entity
		entity.Draw();
	}

	// draw the sky
	sky->Draw(cameras[activeCamera]);

	// Post Processing
	{	
		if (ppOptions.postProcessEnabled)
		{
			// Back to the screen (no depth buffer necessary at this point)
			Graphics::Context->OMSetRenderTargets(1, Graphics::BackBufferRTV.GetAddressOf(), 0);

			// Activate shaders and bind resources
			Graphics::Context->VSSetShader(ppVS.Get(), 0, 0);
			Graphics::Context->PSSetShader(ppPS.Get(), 0, 0);
			Graphics::Context->PSSetShaderResources(0, 1, ppSRV.GetAddressOf());
			Graphics::Context->PSSetSamplers(0, 1, ppSampler.GetAddressOf());

			struct PPData
			{
				float pixelWidth;
				float pixelHeight;
				int blurDistance;
			};

			PPData psData = {};
			psData.pixelWidth = 1.0f / Window::Width();
			psData.pixelHeight = 1.0f / Window::Height();
			psData.blurDistance = 1;
			Graphics::FillAndBindNextConstantBuffer(&psData, sizeof(PPData), D3D11_PIXEL_SHADER, 0);

			// Draw exactly 3 vertices
			Graphics::Context->Draw(3, 0);
		}
	}

	// Unbind all SRVs at the end of the frame so they won't be bound as inputs next frame
	ID3D11ShaderResourceView* nullSRVs[128] = {};
	Graphics::Context->PSSetShaderResources(0, 128, nullSRVs);

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}

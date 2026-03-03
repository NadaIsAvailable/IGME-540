#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"

#include <string>
#include <DirectXMath.h>

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
	textInput("edit this text")
{
	// Load shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader> basicVS = LoadVertexShader(L"VertexShader.cso");
	Microsoft::WRL::ComPtr<ID3D11PixelShader> basicPS = LoadPixelShader(L"PixelShader.cso");

	// Material class testing
	std::vector<std::shared_ptr<Material>> materials;

	// Make materials
	materials.push_back(std::make_shared<Material>("Red tint", XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), basicPS, basicVS));
	materials.push_back(std::make_shared<Material>("Green tint", XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), basicPS, basicVS));
	materials.push_back(std::make_shared<Material>("Blue tint", XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), basicPS, basicVS));

	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cube.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/cylinder.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/helix.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/quad_double_sided.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/sphere.obj").c_str()));
	meshes.push_back(std::make_shared<Mesh>(FixPath("../../Assets/Meshes/torus.obj").c_str()));

	// Create some game entities with the meshes and materials we just made
	for (auto& mesh : meshes)
	{
		entities.push_back(GameEntity(mesh, materials[0]));
	}

	// Spread out the entities
	int x = -5.0f;
	for (auto& entity : entities)
	{
		entity.GetTransform()->SetPosition(x, 0.0f, 0.0f);
		x += 3.0f;
	}

	// Create a few cameras and store in vector
	cameras.push_back(std::make_shared<Camera>(
		Window::AspectRatio(), 
		XMFLOAT3(4.0f, 5.5f, -8.0f))
	);
	cameras[0]->GetTransform()->SetRotation(0.5f, 0.0f, 0.0f);

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

	// Create an input layout
	{
		ID3DBlob* vertexShaderBlob;
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create an input layout 
		//  - This describes the layout of data sent to a vertex shader
		//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
		//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
		//  - Luckily, we already have that loaded (the vertex shader blob above)
		D3D11_INPUT_ELEMENT_DESC inputElements[3] = {};

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

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			3,										// How many elements in that array?
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

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Create a constant buffer to send abritary data to the rendering process
	
	// Vertex Shader Constant Buffer
	// Buffer description
	D3D11_BUFFER_DESC vscbd = {};
	vscbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	vscbd.ByteWidth = (sizeof(VSConstantBuffer) + 15) / 16 * 16;
	vscbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vscbd.MiscFlags = 0;
	vscbd.StructureByteStride = 0;
	vscbd.Usage = D3D11_USAGE_DYNAMIC;

	// Create the buffer
	Graphics::Device->CreateBuffer(&vscbd, 0, vsConstantBuffer.GetAddressOf());

	// Bind this constant buffer to the pipeline for
	// vertex shader at index zero (b0)
	//						   starting index, num of buffers, array of data
	Graphics::Context->VSSetConstantBuffers(0, 1, vsConstantBuffer.GetAddressOf());

	// Set some initial data for the constant buffer
	DirectX::XMStoreFloat4x4(&vsData.world, XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&vsData.view, XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&vsData.projection, XMMatrixIdentity());

	// Pixel Shader Constant Buffer
	// Buffer description
	D3D11_BUFFER_DESC pscbd = {};
	pscbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	pscbd.ByteWidth = (sizeof(PSConstantBuffer) + 15) / 16 * 16;
	pscbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pscbd.MiscFlags = 0;
	pscbd.StructureByteStride = 0;
	pscbd.Usage = D3D11_USAGE_DYNAMIC;

	// Create the buffer
	Graphics::Device->CreateBuffer(&pscbd, 0, psConstantBuffer.GetAddressOf());

	// Bind this constant buffer to the pipeline for
	Graphics::Context->PSSetConstantBuffers(0, 1, psConstantBuffer.GetAddressOf());
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
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::CreateInputLayout()
{
	ID3DBlob* vertexShaderBlob;
	D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

	// Set up the first element - a position, which is 3 float values
	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
	inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
	inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

	// Set up the second element - a color, which is 4 more float values
	inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
	inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
	inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

	// Create the input layout, verifying our description against actual shader code
	Graphics::Device->CreateInputLayout(
		inputElements,							// An array of descriptions
		2,										// How many elements in that array?
		vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
		vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
		inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	
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

	cameras[activeCamera]->Update(deltaTime);
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
	//if (ImGui::CollapsingHeader("Meshes"))
	//{
	//	for (auto& mesh : meshes) 
	//	{
	//		// header for each mesh
	//		std::string header = "Mesh: " + mesh->GetName();
	//		if (ImGui::CollapsingHeader(header.c_str()))
	//		{
	//			// mesh info
	//			ImGui::Text("Triangles: %i", mesh->GetIndexCount() / 3);
	//			ImGui::Text("Vertices: %i", mesh->GetVertexCount());
	//			ImGui::Text("Indices: %i", mesh->GetIndexCount());
	//		}
	//	}
	//}

	// Assignment 04 - Constant Buffer
	//if (ImGui::CollapsingHeader("Constant Buffer")) 
	//{
	//	// Color tint editor
	//	ImGui::ColorEdit4("Color Tint Editor", &vsData.colorTint.x);

	//	// Offset editor
	//	ImGui::DragFloat3("Offset Editor", &vsData.offset.x, 0.01f);
	//}

	// Assignment 05 - Game Entity and Transform
	if (ImGui::CollapsingHeader("Scene Entities"))
	{
		for (uint i = 0; i < entities.size(); i++)
		{
			// header for each mesh
			std::string header = "Entity " + std::to_string(i) +
				'(' + entities[i].GetMesh()->GetName() + ')';
			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::PushID(i);

				XMFLOAT3 pos = entities[i].GetTransform()->GetPosition();
				ImGui::DragFloat3("Position", &pos.x, 0.01f);
				entities[i].GetTransform()->SetPosition(pos);

				XMFLOAT3 rot = entities[i].GetTransform()->GetPitchYawRoll();
				ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f);
				entities[i].GetTransform()->SetRotation(rot);

				XMFLOAT3 scale = entities[i].GetTransform()->GetScale();
				ImGui::DragFloat3("Scale", &scale.x, 0.01f);
				entities[i].GetTransform()->SetScale(scale);

				ImGui::PopID();
			}
		}
	}

	// Assignment 06 - Camera
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
			// header for each mesh
			std::string header = "Camera " + std::to_string(i);
			if (ImGui::CollapsingHeader(header.c_str()))
			{
				ImGui::PushID(i);

				// Position
				XMFLOAT3 pos = cameras[i]->GetTransform()->GetPosition();
				ImGui::DragFloat3("Position", &pos.x, 0.01f);
				cameras[i]->GetTransform()->SetPosition(pos);

				// Rotation
				XMFLOAT3 rot = cameras[i]->GetTransform()->GetPitchYawRoll();
				ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f);
				cameras[i]->GetTransform()->SetRotation(rot);

				// FOV
				float fov = cameras[i]->GetFov();
				ImGui::DragFloat("Field of View (Radians)", &fov, 0.01f, 0.01f, XM_PI);
				cameras[i]->SetFov(fov);

				// Near plane
				float nearPlane = cameras[i]->GetNearPlane();
				ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, cameras[i]->GetFarPlane());
				cameras[i]->SetNearPlane(nearPlane);

				// Far plane
				float farPlane = cameras[i]->GetFarPlane();
				ImGui::DragFloat("Far Plane", &farPlane, 1.0f, cameras[i]->GetNearPlane(), 1000.0f);
				cameras[i]->SetFarPlane(farPlane);

				// Movement speed
				float movementSpeed = cameras[i]->GetMovementSpeed();
				ImGui::DragFloat("Movement Speed", &movementSpeed, 0.01f, 0.01f, 10.0f);
				cameras[i]->SetMovementSpeed(movementSpeed);

				// Look speed
				float lookSpeed = cameras[i]->GetLookSpeed();
				ImGui::DragFloat("Look Speed", &lookSpeed, 0.001f, 0.001f, 0.05f);
				cameras[i]->SetLookSpeed(lookSpeed);

				ImGui::PopID();
			}
		}
	}

	// End custom ui window
	ImGui::End();
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> Game::LoadPixelShader(std::wstring filePath)
{
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;

	// Temporary variable to hold the resulting shader
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts

	// Read our compiled shader code files into blobs
	// - Essentially just "open the file and plop its contents here"
	// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
	D3DReadFileToBlob(FixPath(filePath).c_str(), &pixelShaderBlob);

	// Create the actual Direct3D shaders on the GPU
	Graphics::Device->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
		pixelShaderBlob->GetBufferSize(),		// How big is that data?
		0,										// No classes in this shader
		pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

	return pixelShader;
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> Game::LoadVertexShader(std::wstring filePath)
{
	// Used to store raw data from external files in blob
	ID3DBlob* vertexShaderBlob;

	// Temporary variable to hold the resulting shader
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;

	// Read the compiled shader code file into a blob
	D3DReadFileToBlob(FixPath(filePath).c_str(), &vertexShaderBlob);

	// Create the actual Direct3D vertex shader on the GPU
	Graphics::Device->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
		vertexShaderBlob->GetBufferSize(),		// How big is that data?
		0,										// No classes in this shader
		vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer

	return vertexShader;
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
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	/*
	{
		// Set buffers in the input assembler (IA) stage
		//  - Do this ONCE PER OBJECT, since each object may have different geometry
		//  - For this demo, this step *could* simply be done once during Init()
		//  - However, this needs to be done between EACH DrawIndexed() call
		//     when drawing different geometry, so it's here as an example
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		Graphics::Context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		Graphics::Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		// Tell Direct3D to draw
		//  - Begins the rendering pipeline on the GPU
		//  - Do this ONCE PER OBJECT you intend to draw
		//  - This will use all currently set Direct3D resources (shaders, buffers, etc)
		//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
		//     vertices in the currently set VERTEX BUFFER
		Graphics::Context->DrawIndexed(
			3,     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}
	*/

	// Update the view and projection matrices in the constant buffer data
	// (Same for all entities)
	vsData.view = cameras[activeCamera]->GetView();
	vsData.projection = cameras[activeCamera]->GetProjection();

	// Draw all entities
	for (auto& entity : entities)
	{
		// Activate the shaders for this material
		Graphics::Context->VSSetShader(entity.GetMaterial()->GetVertexShader().Get(), 0, 0);
		Graphics::Context->PSSetShader(entity.GetMaterial()->GetPixelShader().Get(), 0, 0);

		// Rotate each entity a little
		entity.GetTransform()->Rotate(0, 0, (float)sin(deltaTime * 0.5f));

		// Get the world matrix for this entity and store it in the constant buffer data
		vsData.world = entity.GetTransform()->GetWorldMatrix();

		// 
		psData.colorTint = entity.GetMaterial()->GetColorTint();

		// Map the buffer to get its raw memory address
		D3D11_MAPPED_SUBRESOURCE map;

		// Vertex Shader Constant Buffer
		// Map the buffer to get its raw memory address
		Graphics::Context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

		// Copy the data
		memcpy(map.pData, &vsData, sizeof(VSConstantBuffer));

		// Unmap the buffer
		Graphics::Context->Unmap(vsConstantBuffer.Get(), 0);

		// Pixel Shader Constant Buffer
		// Map the buffer to get its raw memory address
		Graphics::Context->Map(psConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

		// Copy the data
		memcpy(map.pData, &psData, sizeof(PSConstantBuffer));

		// Unmap the buffer
		Graphics::Context->Unmap(psConstantBuffer.Get(), 0);

		// Draw the entity
		entity.Draw();
	}

	ImGui::Render(); // Turns this frame’s UI into renderable triangles
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
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

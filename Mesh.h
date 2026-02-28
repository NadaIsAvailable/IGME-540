#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "Vertex.h"

#define uint unsigned int

class Mesh
{
private:
	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Information about the mesh
	std::string displayName;
	uint indexCount;
	uint vertexCount;

public:
	Mesh(std::string name, Vertex* vertices, uint vertCount, uint* indices, uint idxCount);
	Mesh(std::string name, const char* objFile);
	Mesh(const char* objFile);
	~Mesh();

	// Getters
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetIndexCount();
	int GetVertexCount();
	std::string GetName();

	// Helper methods
	void CreateBuffers(Vertex* vertices, uint* indices);

	void Draw();
};


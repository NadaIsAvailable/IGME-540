#pragma once

#include <memory>

#include "Transform.h"
#include "Mesh.h"

class GameEntity
{
private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;

public:
	GameEntity(std::shared_ptr<Mesh> mesh);

	// Getters
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();

	void Draw();
};


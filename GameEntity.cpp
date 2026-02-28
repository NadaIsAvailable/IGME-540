#include "GameEntity.h"
#include "Graphics.h"

GameEntity::GameEntity(
	std::shared_ptr<Mesh> mesh, 
	std::shared_ptr<Material> material) :
	mesh(mesh),
	material(material)
{
	this->transform = std::make_shared<Transform>();
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform()
{
	return transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial()
{
	return material;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> material)
{
	this->material = material;
}

void GameEntity::Draw()
{
	mesh->Draw();
}
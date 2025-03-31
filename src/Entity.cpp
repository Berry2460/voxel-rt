#include "Entity.hpp"

Entity::Entity(int xdim, int ydim, int zdim){
	this->voxels=nullptr;
	this->xdim = xdim;
	this->ydim = ydim;
	this->zdim = zdim;
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->rot = 0.0f;
}

Entity::~Entity(){
	delete this->voxels;
}

void Entity::setPos(int x, int y, int z){
	this->x = x;
	this->y = y;
	this->z = z;
}

void Entity::move(){
	//test
	int voxel = 0x00AA0000;
	
	destroyVoxel((int)this->x + xdim/2, (int)this->y + ydim/2, (int)this->z + zdim/2);
	
	this->z+=2.0f/(float)fps;
	
	placeVoxel((int)this->x + this->xdim/2, (int)this->y + this->ydim/2, (int)this->z + this->zdim/2, voxel);
}

void Entity::update(){
	glm::vec3 posStart = glm::vec3(this->x, this->y, this->z);
	glm::vec3 posEnd = glm::vec3(this->x+this->xdim, this->y+this->ydim, this->z+this->zdim);
	updatePartialGeometry(posStart, posEnd);
}

void Entity::setVoxels(int* voxelData){
	this->voxels = voxelData;
}

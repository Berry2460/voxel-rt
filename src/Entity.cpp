#include "Entity.hpp"

Entity::Entity(int xdim, int ydim, int zdim){
	int toAlloc=xdim*ydim*zdim;
	this->voxels=new int[toAlloc];
	this->xdim=xdim;
	this->ydim=ydim;
	this->zdim=zdim;
	this->x=0;
	this->y=0;
	this->z=0;
	this->xrot=0.0f;
	this->yrot=0.0f;
}

Entity::~Entity(){
	delete this->voxels;
}

Entity::move(int x, int y, int z){
	this->x=x;
	this->y=y;
	this->z=z;
}

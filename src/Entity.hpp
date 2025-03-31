#pragma once

#include "render.hpp"

extern bool* entityMap;
extern int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
extern long long fps;

class Entity{
	public:
		Entity(int xdim, int ydim, int zdim);
		~Entity();
		void move();
		void setVoxels(int* voxelData);
		void setPos(int x, int y, int z);
		void update();
	private:
		float rot;
		float x;
		float y;
		float z;
		int xdim;
		int ydim;
		int zdim;
		int* voxels;
};

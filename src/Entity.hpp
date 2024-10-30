#pragma once

extern bool* entityMap;

class Entity{
	public:
		Entity(int xdim, int ydim, int zdim);
		~Entity();
		move(int x, int y, int z);
	private:
		float xrot;
		float yrot;
		int x;
		int y;
		int z;
		int xdim;
		int ydim;
		int zdim;
		int* voxels;
};

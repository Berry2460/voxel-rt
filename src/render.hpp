#pragma once
#include "window.hpp"

#define VOXELS_WIDTH 512
#define VOXELS_HEIGHT 96
#define DEPTH_FIELD_RADIUS 5
#define ENTITY_CHUNK_SIZE 32

extern const int NumVertices;

extern glm::vec3 camPos;
extern glm::vec3 camDir;
extern glm::vec2 camRotation;
extern glm::mat4 rotateMatrix;
extern int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
extern glm::vec3 startLightPos;
extern glm::vec3 lightPos;
extern float aspectRatio;
extern float lightRotation;

extern int viewDepthField;

extern bool* entityMap;

void updateGeometry();
void initRender();
int getVoxelIndex(int x, int y, int z);
void placeVoxel(int x, int y, int z, int voxel);
void destroyVoxel(int x, int y, int z);
void fixDepthField(int x, int y, int z);
void lightUpdate();
void updateUniforms();
void removeSphere(glm::ivec3 pos, int radius);
void reshape(int width, int height);

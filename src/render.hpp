#pragma once
#include "window.hpp"

#define VOXELS_WIDTH 768
#define VOXELS_HEIGHT 96

extern const int NumVertices;

extern glm::vec3 camPos;
extern glm::vec3 camDir;
extern glm::vec2 camRotation;
extern glm::mat4 rotateMatrix;
extern int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
extern glm::vec3 startLightPos;
extern glm::vec3 lightPos;
extern float voxelSize;
extern float aspectRatio;
extern float lightRotation;

void initRender();
void lightUpdate();
void updateUniforms();
void removeSphere(glm::ivec3 pos, int radius);
void reshape(int width, int height);

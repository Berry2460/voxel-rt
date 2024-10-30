#pragma once
#include "render.hpp"

extern bool* entityMap;

void placeBush(glm::ivec3 pos, glm::ivec3 color, int radius);
void removeSphere(glm::ivec3 pos, int radius);
void placeTrunk(glm::ivec3 pos, glm::ivec3 color, int length);
void updateEntities();
void initVoxels();

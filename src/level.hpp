#pragma once
#include "render.hpp"

void placeBush(glm::ivec3 pos, glm::ivec3 color, int radius);
void removeSphere(glm::ivec3 pos, int radius);
void placeTrunk(glm::ivec3 pos, glm::ivec3 color, int length);
void initVoxels();

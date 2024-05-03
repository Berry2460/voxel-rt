#pragma once
#include "window.hpp"

#define PLAYER_HEIGHT 10

extern float lookX;
extern float lookY;

void movementUpdate();
void doMouseLook();
void doDestroy();
void doGravity();

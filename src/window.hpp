#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define KEYS 9

#define KEY_W 0
#define KEY_S 1
#define KEY_A 2
#define KEY_D 3
#define KEY_T 4
#define SPACE 5
#define SHIFT 6
#define LMB 7
#define RMB 8

extern int frames;
extern long long fps;
extern int mouseX;
extern int mouseY;
extern int screenHeight;
extern int screenWidth;
extern int fullscreen;
extern int vsync;
extern bool keys[KEYS];

int startWindow(char* winTitle);
bool windowLoop();

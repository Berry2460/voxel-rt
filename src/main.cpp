#include "window.hpp"
#include "render.hpp"
#include "controls.hpp"

//window globals
long long fps;
int frames;
int mouseX;
int mouseY;
int screenHeight=600;
int screenWidth=800;
int fullscreen=0;
int vsync=1;
bool keys[KEYS];
bool keysPress[KEYS];

//mouse look
float lookX;
float lookY;

//camera data
glm::vec3 camPos = glm::vec3(195, 45, 155);
glm::vec3 camDir = glm::vec3(0, 0, 1);
glm::vec2 camRotation = glm::vec2(0, 0);
glm::mat4 rotateMatrix = glm::mat4(1.0f);

//voxel rendering data
int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
int voxelChunks[(VOXELS_WIDTH>>CHUNK_FACTOR) * (VOXELS_HEIGHT>>CHUNK_FACTOR) * (VOXELS_WIDTH>>CHUNK_FACTOR)];

glm::vec3 startLightPos = glm::vec3((float)VOXELS_WIDTH / 2.0f, (float)VOXELS_WIDTH * 3.0f, (float)VOXELS_WIDTH / 2.0f);
glm::vec3 lightPos = startLightPos;
float aspectRatio = (float)screenWidth / screenHeight;
float lightRotation = -45.0f;

//game loop
int main(){
	
	startWindow("Voxels");
	
	glewInit();
	initRender();
	
	//render
	while (windowLoop()){
		glDrawArrays(GL_TRIANGLES, 0, NumVertices);
		
		lightUpdate();
		movementUpdate();
		doMouseLook();
		doDestroy();
		
		updateUniforms();
	}
	return 0;
}

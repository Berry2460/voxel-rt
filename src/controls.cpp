#include "controls.hpp"
#include "render.hpp"

const float PI = 3.14159f;

const float MAP_EDGE_OFFSET = 1.0f; //camera must stay within 1 block of map bounds

static float gravity=0.0f;

int collided(){
	int collided=0;
	for (int i=1; i<PLAYER_HEIGHT; i++){
		int index=getVoxelIndex((int)camPos.x, (int)camPos.y - PLAYER_HEIGHT + i, (int)camPos.z);
		if (voxels[index] > -1){
			collided=i;
		}
	}
	return collided;
}


void movementUpdate(){
    float speed = 0.3f;
    glm::mat4 rot=glm::mat4(1.0f);
    rot = glm::rotate(rot, glm::radians(90.0f), glm::vec3(0, 1, 0));
	glm::vec3 frontStep = glm::normalize(glm::vec3(camDir.x, 0, camDir.z)) * speed; //ignore Y axis
    glm::vec3 sideStep = glm::normalize(glm::vec3(glm::vec4(camDir.x, 0, camDir.z, 0) * rot)) * speed;
	
	glm::vec3 stepTaken=glm::vec3(0,0,0);

    if (keys[KEY_W]){
        camPos += frontStep;
		stepTaken+=frontStep;
    }
    if (keys[KEY_S]){
        camPos -= frontStep;
		stepTaken+=-frontStep;
    }
    if (keys[KEY_A]){
        camPos += sideStep;
		stepTaken+=sideStep;
    }
    if (keys[KEY_D]){
        camPos -= sideStep;
		stepTaken+=-sideStep;
    }
	if (keys[SPACE]){
		int index=getVoxelIndex((int)camPos.x, (int)camPos.y - PLAYER_HEIGHT, (int)camPos.z);
		if (voxels[index] > -1 && gravity == 0.0f){
			gravity+=0.4f;
		}
    }
	if (keys[SHIFT]){
		keys[SHIFT]=false;
		viewDepthField=!viewDepthField;
    }
	
	//movement collision check
	int collision=collided();
	while (collision){
		if (stepTaken.x*stepTaken.x + stepTaken.y*stepTaken.y + stepTaken.z*stepTaken.z > 0){
			collision=collided();
			if (collision > PLAYER_HEIGHT>>2){
				camPos-=stepTaken;
			}
			else{
				camPos.y+=collision;
			}
		}
	}
}


void doGravity(){
	camPos.y+=gravity;
	
	//bound camera
    camPos.x = glm::min(glm::max(camPos.x, MAP_EDGE_OFFSET), (float)VOXELS_WIDTH - MAP_EDGE_OFFSET - 1);
    camPos.y = glm::min(glm::max(camPos.y, MAP_EDGE_OFFSET + PLAYER_HEIGHT), (float)VOXELS_HEIGHT - MAP_EDGE_OFFSET - 1);
    camPos.z = glm::min(glm::max(camPos.z, MAP_EDGE_OFFSET), (float)VOXELS_WIDTH - MAP_EDGE_OFFSET - 1);
	
	int index=getVoxelIndex((int)camPos.x, (int)camPos.y - PLAYER_HEIGHT, (int)camPos.z);
	int collision=collided();
	
	//if hit something
	if (collision){
		//undo gravity
		camPos.y-=gravity;
		gravity=0.0f;
	}
	//falling
	else if (voxels[index] < 0){
		gravity-=0.01f;
	}
}

void doDestroy(){
	float destroyRange = 15.0f;
	
	if (keys[RMB]){
		//destroy
		keys[RMB]=false;
		removeSphere(camPos + destroyRange*camDir, destroyRange/2.0f);
	}
}

void doMouseLook(){
	float sensitivity=0.07f;
	lookX=(float)mouseX * 2.0f / screenWidth - 1.0f;
	lookY=(float)mouseY * 2.0f / screenHeight - 1.0f;
	glm::mat4 rotX=glm::mat4(1.0f);
	glm::mat4 rotY=glm::mat4(1.0f);
	
	if (keys[LMB]){
		camRotation += glm::vec2(lookY * sensitivity, lookX * sensitivity);

		if (camRotation.x >= 2 * PI){
			camRotation.x = 0.0f;
		}
		else if (camRotation.x <= -2 * PI){
			camRotation.x = 0.0f;
		}

		if (camRotation.y >= 2 * PI){
			camRotation.y = 0.0f;
		}
		else if (camRotation.y <= -2 * PI){
			camRotation.y = 0.0f;
		}

		camRotation.x = glm::min(PI/2.0f, glm::max(-PI/2.0f, camRotation.x));

		rotX = glm::rotate(rotX, camRotation.x, glm::vec3(1, 0, 0));
		rotY = glm::rotate(rotY, camRotation.y, glm::vec3(0, 1, 0));

		rotateMatrix = rotY * rotX;
		camDir = rotateMatrix * glm::vec4(0, 0, 1, 1);
	}
}

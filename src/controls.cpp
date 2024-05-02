#include "controls.hpp"
#include "render.hpp"

const float PI = 3.14159f;

const float MAP_EDGE_OFFSET = 1.0f; //camera must stay within 1 block of map bounds

void movementUpdate(){
    float speed = 0.6f;
    glm::mat4 rot=glm::mat4(1.0f);
    rot = glm::rotate(rot, glm::radians(90.0f), glm::vec3(0, 1, 0));
    glm::vec3 sideStep = glm::normalize(glm::vec3(glm::vec4(camDir.x, 0, camDir.z, 0) * rot)) * speed; //ignore Y axis on side-steps

    if (keys[KEY_W]) {
        camPos += camDir * speed;
    }
    if (keys[KEY_S]) {
        camPos -= camDir * speed;
    }
    if (keys[KEY_A]) {
        camPos += sideStep;
    }
    if (keys[KEY_D]) {
        camPos -= sideStep;
    }

    //bound camera
    camPos.x = glm::min(glm::max(camPos.x, MAP_EDGE_OFFSET), (float)VOXELS_WIDTH - MAP_EDGE_OFFSET - 1);
    camPos.y = glm::min(glm::max(camPos.y, MAP_EDGE_OFFSET), (float)VOXELS_HEIGHT - MAP_EDGE_OFFSET - 1);
    camPos.z = glm::min(glm::max(camPos.z, MAP_EDGE_OFFSET), (float)VOXELS_WIDTH - MAP_EDGE_OFFSET - 1);
}

void doDestroy(){
	float destroyRange = 15.0f;
	
	if (keys[RMB]){
		//destroy
		keys[RMB]=false;
		removeSphere(camPos + destroyRange*camDir, destroyRange/2.0f);
		updateGeometry();
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

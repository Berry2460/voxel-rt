#version 430

const int VOXELS_WIDTH=512;
const int VOXELS_HEIGHT=96;
const int RENDER_DIST=384;
const int MAX_LOCAL_LIGHTS=16;
const int LOCAL_LIGHT_DIST=64;
const float AMBIENT=0.4f;
const float DIFFUSE=0.8f;
const float MAX_OVERBRIGHT=1.25f;

layout(std430, binding=2) buffer voxelBuffer{
	int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
	
};

in vec4 vPos;
out vec4 fColor;

uniform vec3 camPos;
uniform vec2 camRotation;
uniform vec3 lightPos;
uniform float aspectRatio;
uniform mat4 rotateMatrix;
uniform int viewDepthField;
uniform vec4 localLights[MAX_LOCAL_LIGHTS];

vec3 hitPos=vec3(0,0,0);
vec3 hitNormal=vec3(0,0,0);

float stepCount=0.0f;

int getVoxelIndex(ivec3 currCheck){
	int hit=-1;

	//convert our vector into a 3D array index
	currCheck.y*=VOXELS_WIDTH;
	currCheck.z*=VOXELS_WIDTH * VOXELS_HEIGHT;
	int index=currCheck.x+currCheck.y+currCheck.z;

	//check if the index is within array bounds
	if (index < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
		currCheck.z >= 0 && currCheck.z < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
		currCheck.y >= 0 && currCheck.y < VOXELS_WIDTH*VOXELS_HEIGHT &&
		currCheck.x >= 0 && currCheck.x < VOXELS_WIDTH){

		//hit voxel
		hit=index;
	}
	
	return hit;
}

ivec3 vec3ToIntVec3(vec3 oldVec){
	ivec3 newVec=ivec3(int(oldVec.x), int(oldVec.y), int(oldVec.z));
	return newVec;
}

int castRay(vec3 startPosition, vec3 rayDirection, int dist){ //NOTE: rayDirection should be normalized
	//record which axis rays have hit a plane
	bvec3 axisHit=bvec3(false, false, false);
	
	//storage for ray steps
	ivec3 currCheck=vec3ToIntVec3(startPosition);

	//color index of the ray
	int fColorIndex=-1;
	int tempIndex=-1;

	//calculate the step intervals each axis takes
	ivec3 step=vec3ToIntVec3(vec3(sign(rayDirection.x), sign(rayDirection.y), sign(rayDirection.z)));
	ivec3 forwardSteps=ivec3(int(step.x > 0), int(step.y > 0), int(step.z > 0));
	
	float dx=1.0f/abs(rayDirection.x);
	float dy=1.0f/abs(rayDirection.y);
	float dz=1.0f/abs(rayDirection.z);
	
	//find the first intersect of each axis
	vec3 intersect=(currCheck + forwardSteps - startPosition) / rayDirection;
	
	float currDist=0.0f;
	float distTravelled=0.0f;
	while (distTravelled < dist){
		stepCount++;
		distTravelled++;
		//check which axis has the shortest intersect
		if (intersect.x < intersect.y && intersect.x < intersect.z){
			currDist=intersect.x;
			currCheck.x+=step.x;
			intersect.x+=dx;
			hitNormal=vec3(-step.x, 0, 0);
		}
		else if (intersect.y < intersect.x && intersect.y < intersect.z){
			currDist=intersect.y;
			currCheck.y+=step.y;
			intersect.y+=dy;
			hitNormal=vec3(0, -step.y, 0);
		}
		else{
			currDist=intersect.z;
			currCheck.z+=step.z;
			intersect.z+=dz;
			hitNormal=vec3(0, 0, -step.z);
		}
		tempIndex=getVoxelIndex(currCheck);
		
		//ray hit
		if (tempIndex >= 0 && voxels[tempIndex] >= 0){
			hitPos=rayDirection*currDist + startPosition;
			fColorIndex=tempIndex;
			break;
		}
		//depth field jump
		else if (tempIndex >= 0 && voxels[tempIndex] != -1){
			float toJump=-intBitsToFloat(voxels[tempIndex]);
			distTravelled+=toJump;
			currDist+=toJump;
			startPosition=rayDirection*currDist + startPosition;
			currCheck=vec3ToIntVec3(startPosition);
			intersect=(currCheck + forwardSteps - startPosition) / rayDirection;
		}
		//out of bounds
		else if (tempIndex < 0){
			break;
		}
	}
	
	return fColorIndex;
}

void main(){
	//set background color
	fColor=vec4(0.6, 0.7, 0.8, 1);

	//init our ray direction
	vec3 rayDirection=normalize(vec3(vPos.x*aspectRatio, vPos.y, 1.0f));
	vec3 rotatedDir=vec3(rotateMatrix * vec4(rayDirection, 0));
	
	int fColorIndex=castRay(camPos, rotatedDir, RENDER_DIST);
	vec3 firstHitPos=hitPos;
	vec3 firstHitNormal=hitNormal;
	
	if (viewDepthField == 1){
		fColor=vec4(stepCount/100, stepCount/100, stepCount/100, 1);
	}
	else{
		vec3 toLight=normalize(lightPos - firstHitPos);
		
		float multiplier=AMBIENT;
		
		//apply color of shortest ray
		if (fColorIndex != -1 && voxels[fColorIndex] >= 0){
			//cast shadow ray
			if (castRay(firstHitPos + toLight*0.001f, toLight, RENDER_DIST) == -1){
				multiplier+=DIFFUSE*max(0, dot(firstHitNormal, toLight));
			}
			
			//cast rays to local lights
			for (int i=0; i<MAX_LOCAL_LIGHTS; i++){
				//ensure we don't make areas overbright
				if (multiplier >= MAX_OVERBRIGHT){
					multiplier=MAX_OVERBRIGHT;
					break;
				}
				//ensure local light is in scene
				else if (localLights[i].x >= 0 && localLights[i].y >= 0 && localLights[i].z >= 0){
					float localLightDist=length(localLights[i].xyz - firstHitPos);
					//make sure local light isnt too far away
					if (localLightDist <= LOCAL_LIGHT_DIST){
						//cast ray to local light
						vec3 toLocalLight=normalize(localLights[i].xyz - firstHitPos);
						if (castRay(firstHitPos + toLocalLight*0.001f, toLocalLight, int(localLightDist+1)) == -1){
							//use normal and add light decay for local lights
							multiplier+=localLights[i].a * max(0, dot(firstHitNormal, toLocalLight)) * ((LOCAL_LIGHT_DIST - localLightDist) / LOCAL_LIGHT_DIST);
						}
					}
				}
			}
			
			//color of voxels is stored in a single int to save memory, we use bitwise ops to extract RGB values
			fColor.r=float((voxels[fColorIndex] & 0x00FF0000) >> 16) / 255.0f * multiplier;
			fColor.g=float((voxels[fColorIndex] & 0x0000FF00) >> 8) / 255.0f * multiplier;
			fColor.b=float(voxels[fColorIndex] & 0x000000FF) / 255.0f * multiplier;
			fColor.a=1.0f;
		}
	}
}

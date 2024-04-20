#version 430

const int VOXELS_WIDTH=768;
const int VOXELS_HEIGHT=96;
const int RENDER_DIST=128;
const float AMBIENT=0.4f;
const float DIFFUSE=0.8f;

layout(std430, binding=2) buffer voxelBuffer{
	int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
};

in vec4 vPos;
out vec4 fColor;

uniform float voxelSize;
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec2 camRotation;
uniform vec3 lightPos;
uniform float aspectRatio;
uniform mat4 rotateMatrix;

vec3 hitPos=vec3(0,0,0);;
vec3 hitNormal=vec3(0,0,0);;

int getVoxelIndex(vec3 currCheck, vec3 startPosition, vec3 lookDir){
	int hit=-1;

	//make sure we do not collide with objects behind the camera on accident
	if (dot(currCheck-startPosition, lookDir) > 0){
		//convert our vector into a 3D array index
		int checkX=int(currCheck.x / voxelSize);
		int checkY=int(currCheck.y / voxelSize) * VOXELS_WIDTH;
		int checkZ=int(currCheck.z / voxelSize) * VOXELS_WIDTH * VOXELS_HEIGHT;

		//check contents of index and if the index is within array bounds
		if (checkZ+checkY+checkX < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
			checkZ > 0 && checkZ < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
			checkY > 0 && checkY < VOXELS_WIDTH*VOXELS_HEIGHT &&
			checkX > 0 && checkX < VOXELS_WIDTH &&
			voxels[checkZ+checkY+checkX] != -1){

			//hit voxel
			hit=checkZ+checkY+checkX;
		}
	}
	
	return hit;
}

int castRay(vec3 startPosition, vec3 rayDirection, vec3 lookDir, int dist){ //NOTE: rayDirection should be normalized
	//since we are casting rays in 3D space, we need to cast a ray for each axis
	//each axis ray will only be able to collide with planes on its respective axis
	//after each ray has collided with a plane on its own axis, we take the color value of the shortest ray

	//record which axis rays have hit a plane
	bvec3 axisHit=bvec3(false, false ,false);

	//record what color index each ray hits
	int xColorIndex=-1;
	int yColorIndex=-1;
	int zColorIndex=-1;

	//color index of the shortest ray
	int fColorIndex=-1;
	
	//storage for ray steps
	vec3 currCheckX;
	vec3 currCheckY;
	vec3 currCheckZ;

	//store ray lengths to test out of bounds
	float rayLength=RENDER_DIST;
	float rayLengthX=rayLength;
	float rayLengthY=rayLength;
	float rayLengthZ=rayLength;

	//calculate the step intervals each ray takes
	float dx=voxelSize/rayDirection.x;
	float dy=voxelSize/rayDirection.y;
	float dz=voxelSize/rayDirection.z;

	//find the first intersect of each ray
	vec3 firstX=(int(startPosition.x/voxelSize) - startPosition.x/voxelSize + 0.0001f*sign(rayDirection.x)) * dx * rayDirection + startPosition;
	vec3 firstY=(int(startPosition.y/voxelSize) - startPosition.y/voxelSize + 0.0001f*sign(rayDirection.y)) * dy * rayDirection + startPosition;
	vec3 firstZ=(int(startPosition.z/voxelSize) - startPosition.z/voxelSize + 0.0001f*sign(rayDirection.z)) * dz * rayDirection + startPosition;

	//cast X plane ray
	for (int i=0; i*dx*sign(dx) < dist && !axisHit.x; i++){
		if (!axisHit.x){
			currCheckX=i*dx*sign(rayDirection.x)*rayDirection + firstX;
			rayLengthX=length(currCheckX - startPosition);
			xColorIndex=getVoxelIndex(currCheckX, startPosition, lookDir);
			
			//test out of bounds
			axisHit.x=(rayLengthX > dist ||
						(currCheckX.x < 0 && currCheckX.x >= VOXELS_WIDTH) ||
						(currCheckX.y < 0 && currCheckX.y >= VOXELS_HEIGHT) ||
						(currCheckX.z < 0 && currCheckX.z >= VOXELS_WIDTH));

			//test hit
			if (xColorIndex != -1 && !axisHit.x){
				axisHit.x=true;

				//test shortest ray
				if (rayLengthX < rayLength){
					hitNormal=vec3(-sign(rayDirection.x), 0, 0);
					fColorIndex=xColorIndex;
					hitPos=currCheckX;
					rayLength=rayLengthX;
				}
			}
		}
	}

	//cast Y plane ray
	for (int i=0; i*dy*sign(dy) < dist && !axisHit.y; i++){
		if (!axisHit.y){
			currCheckY=i*dy*sign(rayDirection.y)*rayDirection + firstY;
			rayLengthY=length(currCheckY - startPosition);
			yColorIndex=getVoxelIndex(currCheckY, startPosition, lookDir);
			
			//test out of bounds
			axisHit.y=(rayLengthY > dist ||
						(currCheckY.x < 0 && currCheckY.x >= VOXELS_WIDTH) ||
						(currCheckY.y < 0 && currCheckY.y >= VOXELS_HEIGHT) ||
						(currCheckY.z < 0 && currCheckY.z >= VOXELS_WIDTH));

			//test hit
			if (yColorIndex != -1 && !axisHit.y){
				axisHit.y=true;

				//test shortest ray
				if (rayLengthY < rayLength){
					hitNormal=vec3(0, -sign(rayDirection.y), 0);
					fColorIndex=yColorIndex;
					hitPos=currCheckY;
					rayLength=rayLengthY;
				}
			}
		}
	}

	//cast Z plane ray
	for (int i=0; i*dz*sign(dz) < dist && !axisHit.z; i++){
		if (!axisHit.z){
			currCheckZ=i*dz*sign(rayDirection.z)*rayDirection + firstZ;
			rayLengthZ=length(currCheckZ - startPosition);
			zColorIndex=getVoxelIndex(currCheckZ, startPosition, lookDir);
			
			//test out of bounds
			axisHit.z=(rayLengthZ > dist ||
						(currCheckZ.x < 0 && currCheckZ.x >= VOXELS_WIDTH) ||
						(currCheckZ.y < 0 && currCheckZ.y >= VOXELS_HEIGHT) ||
						(currCheckZ.z < 0 && currCheckZ.z >= VOXELS_WIDTH));

			//test hit
			if (zColorIndex != -1 && !axisHit.z){
				axisHit.z=true;

				//test shortest ray
				if (rayLengthZ < rayLength){
					hitNormal=vec3(0, 0, -sign(rayDirection.z));
					fColorIndex=zColorIndex;
					hitPos=currCheckZ;
					rayLength=rayLengthZ;
				}
			}
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

	int fColorIndex=castRay(camPos*voxelSize, rotatedDir, camDir, RENDER_DIST);

	
	vec3 toLight=normalize(lightPos - hitPos);

	float multiplier=AMBIENT;

	//apply color of shortest ray
	if (fColorIndex != -1){
		//cast shadow ray
		if (castRay(hitPos + toLight, toLight, toLight, RENDER_DIST>>2) == -1){
			multiplier=AMBIENT + DIFFUSE*max(0, dot(hitNormal, toLight));
		}
		//color of voxels is stored in a single int to save memory, we use bitwise ops to extract RGB values
		fColor.r=float((voxels[fColorIndex] & 0x00FF0000) >> 16) / 255.0f * multiplier;
		fColor.g=float((voxels[fColorIndex] & 0x0000FF00) >> 8) / 255.0f * multiplier;
		fColor.b=float(voxels[fColorIndex] & 0x000000FF) / 255.0f * multiplier;
		fColor.a=1.0f;
	}
}

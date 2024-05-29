#version 430

#define DEPTH_DEBUG

const int VOXELS_WIDTH=512;
const int VOXELS_HEIGHT=96;

const float RENDER_DIST=384.0f;
const float AMBIENT=0.4f;
const float DIFFUSE=0.8f;

layout(std430, binding=2) buffer voxelBuffer{
	int voxels[VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH];
	
};

in vec4 vPos;
out vec4 fColor;

uniform vec3 camPos;
uniform vec3 camDir;
uniform vec2 camRotation;
uniform vec3 lightPos;
uniform float aspectRatio;
uniform mat4 rotateMatrix;
uniform int viewDepthField;

vec3 hitPos=vec3(0,0,0);
vec3 hitNormal=vec3(0,0,0);
float hitDistance = 0.0f;

#ifdef DEPTH_DEBUG
vec3 stepCount=vec3(0,0,0);
#endif

int checkIndex(int X, int Y, int Z){

	Y = Y * VOXELS_WIDTH;
	Z = Z * VOXELS_WIDTH * VOXELS_HEIGHT;
	int total = X+Y+Z;
	//check if the index is within array bounds
	if (total < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
		Z >= 0 && Z < VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH &&
		Y >= 0 && Y < VOXELS_WIDTH*VOXELS_HEIGHT &&
		X >= 0 && X < VOXELS_WIDTH){

		//hit voxel
		return total;
	}
	
	return -1;
}

//https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
//https://github.com/cgyurgyik/fast-voxel-traversal-algorithm/blob/master/amanatidesWooAlgorithm.cpp
//https://github.com/cgyurgyik/fast-voxel-traversal-algorithm/blob/master/overview/FastVoxelTraversalOverview.md
int castRay(vec3 startPosition, vec3 rayDirection, vec3 lookDir, float dist){
	rayDirection = normalize(rayDirection);

	// u->
	int X = int(startPosition.x);
	int Y = int(startPosition.y);
	int Z = int(startPosition.z);

	//v-> = rayDirection
	int stepX = bool(rayDirection.x == 0.0f) ? 0 : int(sign(rayDirection.x)); 
	int stepY = bool(rayDirection.y == 0.0f) ? 0 : int(sign(rayDirection.y)); 
	int stepZ = bool(rayDirection.z == 0.0f) ? 0 : int(sign(rayDirection.z)); 

	float tMaxX = ((X + int(stepX > 0) * 1) - startPosition.x) / rayDirection.x;
	float tMaxY = ((Y + int(stepY > 0) * 1) - startPosition.y) / rayDirection.y;
	float tMaxZ = ((Z + int(stepZ > 0) * 1) - startPosition.z) / rayDirection.z;

	//t = step amount
	float tDeltaX = 1 / abs(rayDirection.x);
	float tDeltaY = 1 / abs(rayDirection.y);
	float tDeltaZ = 1 / abs(rayDirection.z);

	// algorithm = u-> + t * v-> for >= 0 

	float distAcume = 0.0f;
	float curDist = 0.0f;
	int hitIndex = -1;

	bool hitX = false;
	bool hitY = false;
	bool hitZ = false;
   	while(dist > curDist) { 

        if(tMaxX < tMaxY) { 
          if(tMaxX < tMaxZ) { 
             X= X + stepX; 
			 hitX = true;
			 curDist = tMaxX;
             tMaxX= tMaxX + tDeltaX; 

			 #ifdef DEPTH_DEBUG
				stepCount.x++;
			 #endif 
			 	 
          } else { 
             Z= Z + stepZ; 
			 hitZ = true;
			 curDist = tMaxZ;	
             tMaxZ= tMaxZ + tDeltaZ;

			 #ifdef DEPTH_DEBUG
				stepCount.z++;
			 #endif 
			  
          } 
        } else { 
          if(tMaxY < tMaxZ) { 
             Y= Y + stepY; 
			 hitY = true;
			 curDist = tMaxY;
             tMaxY= tMaxY + tDeltaY; 	

			 #ifdef DEPTH_DEBUG
				stepCount.y++;
			 #endif 
			 
          } else { 
             Z= Z + stepZ; 
			 hitZ = true;
			 curDist = tMaxZ;
             tMaxZ= tMaxZ + tDeltaZ; 

			 #ifdef DEPTH_DEBUG
				stepCount.z++;
			 #endif 
			 
          } 
        } 



		hitIndex = checkIndex(X,Y,Z);
		//castRay

		if (hitIndex != -1)
		{

			int color = voxels[hitIndex];
			if (color < -1)
			{
				float stepAmnt = intBitsToFloat(color) * -1.0f; // this the sdf skipping part
				
				if (stepAmnt > 0.0f)
				{
									//stepAmnt *= .5f;
					vec3 newPos = startPosition + rayDirection * (stepAmnt + curDist);
					distAcume += curDist + stepAmnt;
					startPosition = newPos;
					X = int(newPos.x);
					Y = int(newPos.y);
					Z = int(newPos.z);

					tMaxX = ((X + int(stepX > 0) * 1) - newPos.x) / rayDirection.x;
					tMaxY = ((Y + int(stepY > 0) * 1) - newPos.y) / rayDirection.y;
					tMaxZ = ((Z + int(stepZ > 0) * 1) - newPos.z) / rayDirection.z;



					//stepAmnt *= 0.166666f;
					//return  int(flColorToInt(vec4(1, stepAmnt, stepAmnt, stepAmnt)));
				}

			} 
			else if (color >= 0)
			{			
				hitNormal=vec3(int(hitX) * -stepX, int(hitY) * -stepY, int(hitZ) * -stepZ);

			 	hitPos = startPosition + curDist * rayDirection + hitNormal * .001f;
				
				/*int rer = checkIndex(int(hitPos.x), int(hitPos.y), int(hitPos.z));
				if (rer != -1 &&  voxels[rer] >= 0) testing pos inac
					return -1;*/

				hitDistance = curDist + distAcume;
				return color;
			}
				
		}
		else
			return -1;

		hitX = false;
		hitY = false;
		hitZ = false;
    } 
   return -1;
}

void main(){
	//set background color
	fColor=vec4(0.6, 0.7, 0.8, 1);

	//init our ray direction
	vec3 rayDirection=normalize(vec3(vPos.x*aspectRatio, vPos.y, 1.0f));
	vec3 rotatedDir=normalize(vec3(rotateMatrix * vec4(rayDirection, 0)));
	
	int color=castRay(camPos, rotatedDir, camDir, RENDER_DIST);
	
	#ifdef DEPTH_DEBUG
	if (viewDepthField == 1){
		fColor=vec4(stepCount/50, 1);
	}
	else 
	#endif
	{			
		float multiplier=AMBIENT;
		
		//apply color of shortest ray
		if (color != -1){
			vec3 toLight=normalize(lightPos - hitPos);
			//cast shadow ray
			if (castRay(hitPos + toLight*0.001f, toLight, toLight, RENDER_DIST * .5f) == -1){
				multiplier=AMBIENT + DIFFUSE*max(0, dot(hitNormal, toLight));
			}
			
			//color of voxels is stored in a single int to save memory, we use bitwise ops to extract RGB values
			fColor.r=float((color & 0x00FF0000) >> 16) / 255.0f * multiplier;
			fColor.g=float((color & 0x0000FF00) >> 8) / 255.0f * multiplier;
			fColor.b=float(color & 0x000000FF) / 255.0f * multiplier;
			fColor.a=1.0f;
		}
	}
}

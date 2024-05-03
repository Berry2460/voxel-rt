#include "level.hpp"

void placeBush(glm::ivec3 pos, glm::ivec3 color, int radius){
    for (int z = -radius; z < radius; z++) {
        for (int y = -radius; y < radius; y++) {
            for (int x = -radius; x < radius; x++) {
                if (x + pos.x < VOXELS_WIDTH && y + pos.y < VOXELS_HEIGHT && z + pos.z < VOXELS_WIDTH &&
                    x + pos.x >= 0 && y + pos.y >= 0 && z + pos.z >= 0){
                    if (x * x + y * y + z * z < radius * radius){
						destroyVoxel(x, y, z);
						
                        int voxel = 0;
                        voxel += color.r;
                        voxel = voxel << 8;
                        //vary green color of tree
                        voxel += color.g - ((x + y + z) % 3) * 20;
                        voxel = voxel << 8;
                        voxel += color.b;
						
						placeVoxel(x+pos.x, y+pos.y, z+pos.z, voxel);
                    }
                }
            }
        }
    }
}


void removeSphere(glm::ivec3 pos, int radius){
    for (int z = -radius; z < radius; z++) {
        for (int y = -radius; y < radius; y++) {
            for (int x = -radius; x < radius; x++) {
                if (x + pos.x < VOXELS_WIDTH && y + pos.y < VOXELS_HEIGHT && z + pos.z < VOXELS_WIDTH &&
                    x + pos.x >= 0 && y + pos.y >= 0 && z + pos.z >= 0) {
                    if (x * x + y * y + z * z < radius * radius){
						destroyVoxel(x+pos.x, y+pos.y, z+pos.z);
						fixDepthField(x+pos.x, y+pos.y, z+pos.z);
                    }
                }
            }
        }
    }
}


void placeTrunk(glm::ivec3 pos, glm::ivec3 color, int length){
    for (int z = -length>>2; z < length>>2; z++) {
        for (int y = 0; y < length; y++) {
            for (int x = -length>>2; x < length>>2; x++) {
                if (x + pos.x < VOXELS_WIDTH && y + pos.y < VOXELS_HEIGHT && z + pos.z < VOXELS_WIDTH){
					destroyVoxel(x, y, z);
					
					int voxel = 0;
					//vary colors of brown for tree trunk
					voxel += color.r - ((x + z) % 2) * 10;
					voxel = voxel << 8;
					voxel += color.g - ((x + z) % 2) * 10;
					voxel = voxel << 8;
					voxel += color.b;
					
					placeVoxel(x+pos.x, y+pos.y, z+pos.z, voxel);
                }
            }
        }
    }
}


void initVoxels(){
	int voxel;
	
    for (int z = 0; z < VOXELS_WIDTH; z++){
        for (int y = 0; y < VOXELS_HEIGHT; y++){
            for (int x = 0; x < VOXELS_WIDTH; x++){
                //vary colors of ground voxels placed
                int colorVariation = 5 * ((x + y + z) % 3);
				
				destroyVoxel(x, y, z);
				
                //stone
                if (y <= 25){
                    voxel=0;
                    voxel+=90 + colorVariation;
                    voxel=voxel << 8;
                    voxel+=90 + colorVariation;
                    voxel=voxel << 8;
                    voxel+=90 + colorVariation;
					
					placeVoxel(x, y, z, voxel);
                }
                //dirt
                else if (y <= 33){
                    voxel=0;
                    voxel+=120 + colorVariation;
                    voxel=voxel << 8;
                    voxel+=100 + colorVariation;
                    voxel=voxel << 8;
                    voxel+=0;
					
					placeVoxel(x, y, z, voxel);
                }
                //grass
                else if (y <= 36){
                    voxel=0;
                    voxel+=10;
                    voxel=voxel << 8;
                    voxel+=130 + colorVariation;
                    voxel=voxel << 8;
                    voxel+=10;
					
					placeVoxel(x, y, z, voxel);
                }
            }
        }
    }

    for (int z = 10; z < VOXELS_WIDTH-10; z++) {
        for (int x = 10; x < VOXELS_WIDTH-10; x++) {
            if (x % 30 == 0 && z % 25 == 0) {
                placeTrunk(glm::ivec3(x + 1 + z % 7, 36, z), glm::ivec3(128, 100, 15), 6);
                placeBush(glm::ivec3(x + z % 7, 36 + 10, z), glm::ivec3(15, 128, 15), 6);
            }
        }
    }
}

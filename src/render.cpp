#include "render.hpp"
#include "controls.hpp"

#include <iostream>
#include <stdio.h>

const int NumVertices = 6;

// Vertices for fullscreen coverage
glm::vec4 vertices[NumVertices] = {
    glm::vec4(-1, 1, 0, 1),
    glm::vec4(1, -1, 0, 1),
    glm::vec4(1, 1, 0, 1),

    glm::vec4(-1, 1, 0, 1),
    glm::vec4(-1, -1, 0, 1),
    glm::vec4(1, -1, 0, 1),
};

//uniform locations
GLuint ssbo, AspectRatio, VoxelChunks, CamDir, CamPos, CamRotation, LightPos, RotateMatrix;

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE* fp = fopen(shaderFile, "rb");

	if ( fp == NULL ) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);

	buf[size] = '\0';
	fclose(fp);

	return buf;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFile, const char* fShaderFile){
	struct Shader{
		const char* filename;
		GLenum type;
		GLchar* source;
	}
	
	shaders[2] = {
		{vShaderFile, GL_VERTEX_SHADER, NULL},
		{fShaderFile, GL_FRAGMENT_SHADER, NULL}
	};

	GLuint program = glCreateProgram();

	for (int i = 0; i < 2; ++i){
	Shader& s = shaders[i];
	s.source = readShaderSource( s.filename );
	if ( shaders[i].source == NULL ) {
		std::cerr << "Failed to read " << s.filename << std::endl;
		exit( EXIT_FAILURE );
	}

	GLuint shader = glCreateShader( s.type );
	glShaderSource(shader, 1, (const GLchar**) &s.source, NULL);
	glCompileShader(shader);

	GLint  compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	
	if ( !compiled ) {
		std::cerr << s.filename << " failed to compile:" << std::endl;
		GLint  logSize;
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logSize );
		char* logMsg = new char[logSize];
		glGetShaderInfoLog( shader, logSize, NULL, logMsg );
		std::cerr << logMsg << std::endl;
		delete [] logMsg;

		exit( EXIT_FAILURE );
	}
	delete [] s.source;

	glAttachShader( program, shader );
	}

	/* link  and error check */
	glLinkProgram(program);

	GLint  linked;
	glGetProgramiv( program, GL_LINK_STATUS, &linked );
	if ( !linked ) {
		std::cerr << "Shader program failed to link" << std::endl;
		GLint  logSize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog( program, logSize, NULL, logMsg );
		std::cerr << logMsg << std::endl;
		delete [] logMsg;

		exit( EXIT_FAILURE );
	}

	/* use program object */
	glUseProgram(program);

	return program;
}


void updateGeometry(){
	//reload data to SSBO
	//this is inefficient but it works
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
	
	//update chunk data
	glUniform1iv(VoxelChunks, sizeof(voxelChunks), voxelChunks);
}


void placeVoxel(int x, int y, int z, int voxel){
	int index = x + (VOXELS_WIDTH * y) + (VOXELS_WIDTH * VOXELS_HEIGHT * z);
	
	if (index >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		int chunkIndex = (x>>CHUNK_FACTOR) + ((VOXELS_WIDTH>>CHUNK_FACTOR) * (y>>CHUNK_FACTOR)) + ((VOXELS_WIDTH>>CHUNK_FACTOR) * (VOXELS_HEIGHT>>CHUNK_FACTOR) * (z>>CHUNK_FACTOR));
		
		if (voxels[index] == -1){
			voxelChunks[chunkIndex]++;
		}
		voxels[index]=voxel;
	}
}


void destroyVoxel(int x, int y, int z){
	int index = x + (VOXELS_WIDTH * y) + (VOXELS_WIDTH * VOXELS_HEIGHT * z);
	
	if (index >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		int chunkIndex = (x>>CHUNK_FACTOR) + ((VOXELS_WIDTH>>CHUNK_FACTOR) * (y>>CHUNK_FACTOR)) + ((VOXELS_WIDTH>>CHUNK_FACTOR) * (VOXELS_HEIGHT>>CHUNK_FACTOR) * (z>>CHUNK_FACTOR));
		
		if (voxels[index] != -1 && voxelChunks[chunkIndex] > 0){
			voxelChunks[chunkIndex]--;
		}
		voxels[index] = -1;
	}
}


void placeBush(glm::ivec3 pos, glm::ivec3 color, int radius){
    for (int z = -radius; z < radius; z++) {
        for (int y = -radius; y < radius; y++) {
            for (int x = -radius; x < radius; x++) {
                if (x + pos.x < VOXELS_WIDTH && y + pos.y < VOXELS_HEIGHT && z + pos.z < VOXELS_WIDTH &&
                    x + pos.x >= 0 && y + pos.y >= 0 && z + pos.z >= 0){
                    if (x * x + y * y + z * z < radius * radius){
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
                    }
                }
            }
        }
    }
}


void placeTrunk(glm::ivec3 pos, glm::ivec3 color, int length) {
    for (int z = -length>>2; z < length>>2; z++) {
        for (int y = 0; y < length; y++) {
            for (int x = -length>>2; x < length>>2; x++) {
                if (x + pos.x < VOXELS_WIDTH && y + pos.y < VOXELS_HEIGHT && z + pos.z < VOXELS_WIDTH) {
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


void clearChunk(int x, int y, int z){
	int chunkIndex = x + ((VOXELS_WIDTH/CHUNK_FACTOR) * y) + ((VOXELS_WIDTH/CHUNK_FACTOR) * (VOXELS_HEIGHT/CHUNK_FACTOR) * z);
	voxelChunks[chunkIndex]=0;
}


void initVoxelChunks(){
    for (int z = 0; z < VOXELS_WIDTH>>CHUNK_FACTOR; z++){
        for (int y = 0; y < VOXELS_HEIGHT>>CHUNK_FACTOR; y++){
            for (int x = 0; x < VOXELS_WIDTH>>CHUNK_FACTOR; x++){
				clearChunk(x, y, z);
			}
		}
	}
}


void printChunks(){
    for (int z = 0; z < VOXELS_WIDTH>>CHUNK_FACTOR; z++){
        for (int y = 0; y < VOXELS_HEIGHT>>CHUNK_FACTOR; y++){
            for (int x = 0; x < VOXELS_WIDTH>>CHUNK_FACTOR; x++){
				int chunkIndex = x + ((VOXELS_WIDTH/CHUNK_FACTOR) * y) + ((VOXELS_WIDTH/CHUNK_FACTOR) * (VOXELS_HEIGHT/CHUNK_FACTOR) * z);
				printf("chunk at %d %d %d has %d voxels\n", x,y,z,voxelChunks[chunkIndex]);
			}
		}
	}
}


void initVoxels(){
	initVoxelChunks();
	int voxel;
	
	for (int i=0; i<sizeof(voxels)/sizeof(int); i++){
		voxels[i]=-1;
	}
	
    for (int z = 0; z < VOXELS_WIDTH; z++){
        for (int y = 0; y < VOXELS_HEIGHT; y++){
            for (int x = 0; x < VOXELS_WIDTH; x++){
                //vary colors of ground voxels placed
                int colorVariation = 5 * ((x + y + z) % 3);
				
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
	
	//printChunks();
}


void updateUniforms(){
    glUniform1f(AspectRatio, aspectRatio);
    glUniform3f(CamPos, camPos.x, camPos.y, camPos.z);
    glUniform3f(CamDir, camDir.x, camDir.y, camDir.z);
    glUniform2f(CamRotation, camRotation.x, camRotation.y);
    glUniform3f(LightPos, lightPos.x, lightPos.y, lightPos.z);
    glUniformMatrix4fv(RotateMatrix, 1, GL_FALSE, glm::value_ptr(rotateMatrix));
}


void initRender(){
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	// Load shaders and use the resulting shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// set up vertex arrays
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Retrieve transformation uniform variable locations
	CamPos = glGetUniformLocation(program, "camPos");
	CamDir = glGetUniformLocation(program, "camDir");
	CamRotation = glGetUniformLocation(program, "camRotation");
	AspectRatio = glGetUniformLocation(program, "aspectRatio");
	LightPos = glGetUniformLocation(program, "lightPos");
	RotateMatrix = glGetUniformLocation(program, "rotateMatrix");
	
	//init voxels
	initVoxels();
	
	//init uniforms
	updateUniforms();
	glUniform1iv(VoxelChunks, sizeof(voxelChunks), voxelChunks);
	
	//load voxels into GPU
	glGenBuffers(2, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
	
	glShadeModel(GL_FLAT);
}


void lightUpdate(){
    float increment = 0.01f;
    glm::mat4 rot=glm::mat4(1.0f);

    if (lightRotation >= 360.0f) {
        lightRotation = 0.0f; //wrap back to 0 after 360 degrees
    }
    else if (lightRotation >= 180.0f && lightRotation < 270.0f) {
        increment *= 8.0f; //night cycles are fast (to show off the sunrise/sunset quicker)
    }
    lightRotation += increment;
    
    rot = glm::rotate(rot, glm::radians(lightRotation), glm::vec3(0, 0, 1));
    lightPos = rot * glm::vec4(startLightPos, 1);
}

void reshape(int width, int height){
    glViewport(0, 0, width, height);
    screenHeight = height;
    screenWidth = width;

    aspectRatio = (float)width / height;
    glUniform1f(AspectRatio, aspectRatio);
}

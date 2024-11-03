#include "render.hpp"
#include "controls.hpp"
#include "level.hpp"
#include "Entity.hpp"

#include <pthread.h>
#include <atomic>
#include <iostream>

#define THREAD_COUNT 4

const int NumVertices = 6;

struct depthThreadData{
	int start;
	int end;
};

struct depthIndexData{
	float dist;
	int x;
	int y;
	int z;
};

static struct depthIndexData* computeDepthIndices();
//we generate our depth indices first to save us depth calculation time later
static struct depthIndexData* depthIndices=computeDepthIndices();
static int depthIndexCount=0;
static pthread_t genThread[THREAD_COUNT];
struct depthThreadData threadData[THREAD_COUNT];
std::atomic<int> depthGenerationDone;

// Vertices for fullscreen coverage
glm::vec4 vertices[NumVertices] = {
    glm::vec4(-1, 1, 0, 1),
    glm::vec4(1, -1, 0, 1),
    glm::vec4(1, 1, 0, 1),

    glm::vec4(-1, 1, 0, 1),
    glm::vec4(-1, -1, 0, 1),
    glm::vec4(1, -1, 0, 1),
};

//Entity testing
//Entity testE=new Entity(10, 10, 10);
//testE.move(25,25,20);

//uniform locations
GLuint ssbo, AspectRatio, CamPos, CamRotation, LightPos, RotateMatrix, ViewDepthField;

//generate initial indices list for depth optmizations
static struct depthIndexData* computeDepthIndices(){
	float dist=-DEPTH_FIELD_RADIUS+1;
	int entries=0;
	struct depthIndexData data[DEPTH_FIELD_RADIUS*DEPTH_FIELD_RADIUS*DEPTH_FIELD_RADIUS*2*2*2];
	struct depthIndexData* finalData;
	
	for (int zCheck=-DEPTH_FIELD_RADIUS; zCheck<=DEPTH_FIELD_RADIUS; zCheck++){
		for (int yCheck=-DEPTH_FIELD_RADIUS; yCheck<=DEPTH_FIELD_RADIUS; yCheck++){
			for (int xCheck=-DEPTH_FIELD_RADIUS; xCheck<=DEPTH_FIELD_RADIUS; xCheck++){
				if (xCheck*xCheck + yCheck*yCheck + zCheck*zCheck <= DEPTH_FIELD_RADIUS*DEPTH_FIELD_RADIUS){
					
					int xDist=xCheck - (xCheck > 0) + (xCheck < 0);
					int yDist=yCheck - (yCheck > 0) + (yCheck < 0);
					int zDist=zCheck - (zCheck > 0) + (zCheck < 0);
					
					dist=-sqrt(xDist*xDist + yDist*yDist + zDist*zDist);
					
					if (dist <= DEPTH_FIELD_RADIUS){
						data[entries].dist=dist;
						data[entries].x=xCheck;
						data[entries].y=yCheck;
						data[entries].z=zCheck;
						entries++;
					}
				}
			}
		}
	}
	finalData=new struct depthIndexData[entries];
	for (int i=0; i<entries; i++){
		finalData[i]=data[i];
	}
	depthIndexCount=entries;
	return finalData;
}


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

	return program;
}


int getVoxelIndex(int x, int y, int z){
	int index=-1;
	
	if (x >= 0 && y >= 0 && z >= 0 && x < VOXELS_WIDTH && y < VOXELS_HEIGHT && z <VOXELS_WIDTH){
		index=x + (VOXELS_WIDTH * y) + (VOXELS_WIDTH * VOXELS_HEIGHT * z);
	}
	return index;
}


void updateGeometry(){
	//reload data to SSBO
	//this is inefficient but it works
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
}


void fixDepthField(int x, int y, int z){
	int index=getVoxelIndex(x, y, z);
	float dist=-DEPTH_FIELD_RADIUS+1;
	float nearestDist=dist;
	
	if (index >= 0 && voxels[index] < 0){
		for (int i=0; i<depthIndexCount; i++){
			dist=depthIndices[i].dist;
			int xCheck=x+depthIndices[i].x;
			int yCheck=y+depthIndices[i].y;
			int zCheck=z+depthIndices[i].z;
			int indexCheck=getVoxelIndex(xCheck, yCheck, zCheck);
			
			//check point is valid
			if (voxels[indexCheck] >= 0 && dist > nearestDist){
				if (dist <= -2.0f){
					nearestDist=dist;
				}
				else{
					nearestDist=0;
				}
			}
		}
		if (nearestDist < 0){
			voxels[index]=*(int*)&nearestDist;
		}
	}
}


void placeVoxel(int x, int y, int z, int voxel){
	int index=getVoxelIndex(x, y, z);
	
	if (index >= 0){
		voxels[index]=voxel;
	}
}


void destroyVoxel(int x, int y, int z){
	int index=getVoxelIndex(x, y, z);
	
	if (x >= 0 && y >= 0 && z >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		voxels[index] = -1;
	}
}

static void* computeDepthField(void* threadData){
	struct depthThreadData* data=(struct depthThreadData*)threadData;
	
	for (int z = data->start; z < data->end; z++){
		for (int y = 0; y < VOXELS_HEIGHT; y++){
			for (int x = 0; x < VOXELS_WIDTH; x++){
				fixDepthField(x, y, z);
			}
		}
	}
	depthGenerationDone++;
	return NULL;
}


void updateUniforms(){
	glUniform1f(AspectRatio, aspectRatio);
	glUniform3f(CamPos, camPos.x, camPos.y, camPos.z);
	glUniform2f(CamRotation, camRotation.x, camRotation.y);
	glUniform3f(LightPos, lightPos.x, lightPos.y, lightPos.z);
	glUniformMatrix4fv(RotateMatrix, 1, GL_FALSE, glm::value_ptr(rotateMatrix));
	glUniform1i(ViewDepthField, viewDepthField);
	
	if (depthGenerationDone == THREAD_COUNT){
		updateGeometry();
		depthGenerationDone=false;
	}
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
	CamRotation = glGetUniformLocation(program, "camRotation");
	AspectRatio = glGetUniformLocation(program, "aspectRatio");
	LightPos = glGetUniformLocation(program, "lightPos");
	RotateMatrix = glGetUniformLocation(program, "rotateMatrix");
	ViewDepthField = glGetUniformLocation(program, "viewDepthField");
	
	//init uniforms
	updateUniforms();
	
	//init voxels
	for (int i=0; i<VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH; i++){
		voxels[i]=-1;
	}
	initVoxels();
	
	//threaded depth field generation
	for (int i=0; i<THREAD_COUNT; i++){
		threadData[i].start=i*(VOXELS_WIDTH/THREAD_COUNT);
		threadData[i].end=(i+1)*(VOXELS_WIDTH/THREAD_COUNT);
		pthread_create(&genThread[i], NULL, computeDepthField, (void*)&threadData[i]);
	}
	
	//load voxels into GPU
	glGenBuffers(2, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
	
	glShadeModel(GL_FLAT);
}


void lightUpdate(){
    float increment = 0.3f / fps;
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

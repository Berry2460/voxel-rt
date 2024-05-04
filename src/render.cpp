#include "render.hpp"
#include "controls.hpp"
#include "level.hpp"

#include <iostream>

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
GLuint ssbo, AspectRatio, CamDir, CamPos, CamRotation, LightPos, RotateMatrix, ViewDepthField;

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


int getVoxelIndex(int x, int y, int z){
	int index = x + (VOXELS_WIDTH * y) + (VOXELS_WIDTH * VOXELS_HEIGHT * z);
	return index;
}


void updateGeometry(){
	//reload data to SSBO
	//this is inefficient but it works
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
}


void fixDepthFieldRecurse(int x, int y, int z, int range){
	int index=getVoxelIndex(x, y, z);

	int middle=range>>1;
	bool done=false;
	
	for (int zCheck=0; zCheck<range && !done; zCheck++){
		for (int yCheck=0; yCheck<range && !done; yCheck++){
			for (int xCheck=0; xCheck<range && !done; xCheck++){
				//snap XY to other side once Z is in the middle portion
				if (yCheck > 0 && yCheck < range-1 && zCheck > 0 && zCheck < range-1){
					yCheck=range-1;
				}
				if (xCheck > 0 && xCheck < range-1 && zCheck > 0 && zCheck < range-1){
					xCheck=range-1;
				}
				int xPoint=xCheck-middle + x;
				int yPoint=yCheck-middle + y;
				int zPoint=zCheck-middle + z;
				int indexCheck=getVoxelIndex(xPoint, yPoint, zPoint);
				
				//check point is valid
				if (xPoint >= 0 && yPoint >=0 && zPoint >=0 && indexCheck < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
					//logic for empty voxel
					if (voxels[index] < 0){
						//check if we found a solid voxel
						if (voxels[indexCheck] > -1){
							done=true;
							voxels[index]=-middle;
						}
					}
					//logic for solid voxel
					else if (voxels[indexCheck] < -middle){
						voxels[indexCheck]=-middle;
					}
					else{
						done=true;
					}
				}
			}
		}
	}
	if (!done && range < MAX_DEPTH_FIELD){
		fixDepthFieldRecurse(x, y, z, range+2);
	}
	//add minDepth if all space was empty and voxel is empty
	else if (!done && voxels[index] < 0){
		voxels[index]=-middle - 1;
	}
}


void fixDepthField(int x, int y, int z){
	fixDepthFieldRecurse(x, y, z, MIN_DEPTH_FIELD);
}


void placeVoxel(int x, int y, int z, int voxel){
	int index=getVoxelIndex(x, y, z);
	
	if (x >= 0 && y >= 0 && z >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		voxels[index]=voxel;
	}
}


void destroyVoxel(int x, int y, int z){
	int index=getVoxelIndex(x, y, z);
	
	if (x >= 0 && y >= 0 && z >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		voxels[index] = -1;
	}
}

void computeDepthField(){
	for (int z = 0; z < VOXELS_WIDTH; z++){
		for (int y = 0; y < VOXELS_HEIGHT; y++){
			for (int x = 0; x < VOXELS_WIDTH; x++){
				fixDepthField(x, y, z);
			}
		}
	}
}


void updateUniforms(){
	glUniform1f(AspectRatio, aspectRatio);
	glUniform3f(CamPos, camPos.x, camPos.y, camPos.z);
	glUniform3f(CamDir, camDir.x, camDir.y, camDir.z);
	glUniform2f(CamRotation, camRotation.x, camRotation.y);
	glUniform3f(LightPos, lightPos.x, lightPos.y, lightPos.z);
	glUniformMatrix4fv(RotateMatrix, 1, GL_FALSE, glm::value_ptr(rotateMatrix));
	glUniform1i(ViewDepthField, viewDepthField);
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
	ViewDepthField = glGetUniformLocation(program, "viewDepthField");
	
	//init uniforms
	updateUniforms();
	
	//init voxels
	for (int i=0; i<VOXELS_WIDTH*VOXELS_HEIGHT*VOXELS_WIDTH; i++){
		voxels[i]=-1;
	}
	initVoxels();
	computeDepthField();
	
	//load voxels into GPU
	glGenBuffers(2, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxels), voxels, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
	
	glShadeModel(GL_FLAT);
}


void lightUpdate(){
    float increment = 0.004f;
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

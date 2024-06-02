#include "render.hpp"
#include "controls.hpp"
#include "level.hpp"

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <unordered_set>

#define SDF_RADIUS 4

struct SDFType
{
	glm::ivec3 pos;
	float distance;
};

std::vector<SDFType> radiusField = {};
std::vector<glm::ivec3> queuedSDFUpdate = {};

void clearSDFQueue(){
	queuedSDFUpdate.clear();
}

int radiusFieldSize = 0;

int ***tempVoxels = nullptr;

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

bool voxelInBounds(int x, int y, int z){
	return (x >= 0 && y >= 0 && z >= 0 && x < VOXELS_WIDTH && y < VOXELS_HEIGHT && z < VOXELS_WIDTH);
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


void placeVoxel(int x, int y, int z, int voxel){
	int index=getVoxelIndex(x, y, z);
	
	if (x >= 0 && y >= 0 && z >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		voxels[index]=voxel;
		queuedSDFUpdate.emplace_back(glm::ivec3{x, y, z});
	}
}


void destroyVoxel(int x, int y, int z){
	int index=getVoxelIndex(x, y, z);
	
	if (x >= 0 && y >= 0 && z >= 0 && index < VOXELS_WIDTH * VOXELS_HEIGHT * VOXELS_WIDTH){
		voxels[index] = -1;
		queuedSDFUpdate.emplace_back(glm::ivec3{x, y, z});
	}
}

std::vector<SDFType> generateSDFOffsets(int radius)
{
	std::vector<SDFType> offsets = {};

	for (int x = -radius; x <= radius; ++x)
	{
		for (int y = -radius; y <= radius; ++y)
		{
			for (int z = -radius; z <= radius; ++z)
			{
				if (x * x + y * y + z * z <= radius * radius)
				{
					float dist = 0.f;

					if (abs(x) > 1 || abs(y) > 1 || abs(z) > 1)
					{
						glm::vec3 offPos = {x, y, z};

						if (x > 0)
							offPos.x -= 1.f;
						else if (x < 0)
							offPos.x += 1.f;

						if (y > 0)
							offPos.y -= 1.f;
						else if (y < 0)
							offPos.y += 1.f;

						if (z > 0)
							offPos.z -= 1.f;
						else if (z < 0)
							offPos.z += 1.f;

						dist = sqrt(offPos.x * offPos.x + offPos.y * offPos.y + offPos.z * offPos.z);
					}

					offsets.emplace_back(SDFType{glm::ivec3(x, y, z), dist});
				}
			}
		}
	}

	std::sort(offsets.begin(), offsets.end(), [](const SDFType &a, const SDFType &b)
			  { return a.distance < b.distance; });

	return offsets;
}

float findFirstNonEmptyVoxel(int startX, int startY, int startZ)
{
	if (voxels[getVoxelIndex(startX,startY,startZ)] >= 0)
		return 0.f;
	for (int i = 0; i < radiusFieldSize; i++)
	{
		const auto &offset = radiusField[i];

		int newX = startX + offset.pos.x;
		int newY = startY + offset.pos.y;
		int newZ = startZ + offset.pos.z;

		if (!voxelInBounds(newX, newY, newZ) || voxels[getVoxelIndex(newX, newY, newZ)] >= 0)
		{
			return offset.distance;
		}
	}

	return SDF_RADIUS - 1;
}

float findFirstNonEmptyVoxelINIT(int startX, int startY, int startZ) // only for init in computeDepthField
{
	if  (tempVoxels[startX][startY][startZ] >= 0)
		return  0.f;
	for (int i = 0; i < radiusFieldSize; i++)
	{
		const auto &offset = radiusField[i];

		int newX = startX + offset.pos.x;
		int newY = startY + offset.pos.y;
		int newZ = startZ + offset.pos.z;

		if (!voxelInBounds(newX, newY, newZ) || tempVoxels[newX][newY][newZ] >= 0)
		{
			return offset.distance;
		}
	}

	return SDF_RADIUS - 1;
}

void computeDepthField(){
	radiusField = generateSDFOffsets(SDF_RADIUS);
	tempVoxels = new int **[VOXELS_WIDTH];
	for (int x = 0; x < VOXELS_WIDTH; x++)
	{
		tempVoxels[x] = new int *[VOXELS_HEIGHT];
		for (int y = 0; y < VOXELS_HEIGHT; y++)
		{
			tempVoxels[x][y] = new int[VOXELS_WIDTH];

			for (int z = 0; z < VOXELS_WIDTH; z++)
			{
				tempVoxels[x][y][z] = voxels[getVoxelIndex(x, y, z)];
			}
		}
	}

	radiusFieldSize = radiusField.size();
	float percent = 1.f / VOXELS_WIDTH;

	for (int z = 0; z < VOXELS_WIDTH; z++)
	{
		for (int y = 0; y < VOXELS_HEIGHT; y++)
		{
			for (int x = 0; x < VOXELS_WIDTH; x++)
			{

				if (tempVoxels[x][y][z] < 0)
				{
					float dist = findFirstNonEmptyVoxelINIT(x, y, z);

					if (dist > 0.f)
					{
						//std::cout << "dist = " << dist << std::endl;
						dist *= -1.f;
						voxels[getVoxelIndex(x, y, z)] = *(int*)&dist;
					}				
				}				
			}			
		}

		std::cout << "sdf generation percent = " << z * percent * 100.0f << std::endl;
	}

	for (int x = 0; x < VOXELS_WIDTH; x++)
	{	
		for (int y = 0; y < VOXELS_HEIGHT; y++)
		{
			delete[] tempVoxels[x][y];
		}
		delete[] tempVoxels[x];
	}
	delete[] tempVoxels;
}

namespace std
{
	template <>
	struct hash<glm::ivec3>
	{
		size_t operator()(const glm::ivec3 &p) const
		{
			return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1) ^ (hash<int>()(p.z) << 2);
		}
	};
}

void runtimeSDFUpdates()
{
	if (!queuedSDFUpdate.empty())
	{
		std::unordered_set<glm::ivec3> sdfCheckSet = {};

		for (const glm::ivec3 &pos : queuedSDFUpdate)
		{
			for (int i = 0; i < radiusFieldSize; i++)
			{
				const auto &offset = radiusField[i];

				int newX = pos.x + offset.pos.x;
				int newY = pos.y + offset.pos.y;
				int newZ = pos.z + offset.pos.z;

				if (voxelInBounds(newX, newY, newZ) && voxels[getVoxelIndex(newX, newY, newZ)] < 0)
				{
					sdfCheckSet.insert(glm::ivec3{newX, newY, newZ}); // so we dont check the same pos multiple times
				}
			}
		}

		int ichk = 0;
		std::vector<int> orderedIndexes(sdfCheckSet.size() + queuedSDFUpdate.size());
		// orderedIndexes.resize(sdfCheckSet.size() + queuedSDFUpdate.size());
		for (const glm::ivec3 &pos : queuedSDFUpdate)
		{
			orderedIndexes[ichk] = getVoxelIndex(pos.x, pos.y, pos.z);
			ichk += 1;
		}

		for (const glm::ivec3 &pos : sdfCheckSet)
		{

			float dist = findFirstNonEmptyVoxel(pos.x, pos.y, pos.z);

			int index = getVoxelIndex(pos.x, pos.y, pos.z);

			if (dist > 0.f)
			{
				// std::cout << "dist = " << dist << std::endl;
				dist *= -1.f;
				voxels[index] = *(int *)&dist;
			}
			else
				voxels[index] = -1;

			orderedIndexes[ichk] = index;

			// glBufferSubData(GL_SHADER_STORAGE_BUFFER, index * 4, 4, &voxels[index]);
			ichk += 1;
		}

		std::sort(orderedIndexes.begin(), orderedIndexes.end(), [](const int &a, const int &b)
				  { return a < b; });

		int lastCheckIndex = orderedIndexes[0];
		int lastStartIndex = lastCheckIndex;

		int chkEnd = ichk - 1;

		for (int i = 0; i < ichk; i++)
		{
			int &curIndex = orderedIndexes[i];
			int chkDelta = curIndex - lastCheckIndex;

			if (i == chkEnd || chkDelta > 1)
			{
				// std::cout << lastStartIndex << std::endl;
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, lastStartIndex * 4, (lastCheckIndex - lastStartIndex) * 4 + 4, &voxels[lastStartIndex]);

				lastCheckIndex = curIndex;
				lastStartIndex = curIndex;
			}
			else
			{
				lastCheckIndex = curIndex;
			}
		}

		queuedSDFUpdate.clear();
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
	//glUseProgram(program); already doing this in InitShader function

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

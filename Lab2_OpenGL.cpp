// ********************************************************************************
// ENTER THE VALUE FOR SIZE OF THE VERTEX ARRAY IN THE CONSOLE TO START THE PROJECT
// ********************************************************************************

#include <iostream>
#include <Windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <IL/il.h>
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_helper.h"

//const int SIZEE = 5;
bool flag = FALSE;
int SIZEE;
float *elevationMap = NULL;
int user_input;
int grid_Size;

//camera
glm::vec3 eyePosition(2.0f, 3.0f, 3.0f);
glm::vec3 lookAt(0.0f, 0.0f, 0.0f);
glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -0.1f);

//window client area size
const int ClientAreaWidth = 800;
const int ClientAreaHeight = 800;

//each vertex is 3 floats for the position, 3 floats for the normal and 2 floats for the texture coordinates

struct Vertex
{
	float position[3];
	float normal[3];
	float texcoord[2];
};



//global variables

//float elevationMap[SIZEE*SIZEE];

GLFWwindow *window;
GLuint gpuProgram;
GLuint vertexBuffer, indexBuffer, vertexArray;
GLuint crateTexture;

//locations of uniform variables in the GPU program
GLuint locMatWorld, locMatView, locMatProjection, locUseTexture, locLightDirection, locTex;

//globals for time measurement
LARGE_INTEGER clockFreq;
LARGE_INTEGER prevTime;

int use_texture = false;
//rotation angle for the cube
float angle = 0.0f;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

float average2(float x, float y)
{
	return (x + y) / 2;
}

float average4(float p, float q, float r, float s)
{
	return (p + q + r + s) / 4;
}

void midPointDisplacement(int a, int b, int c, int d)
{
	float top, bottom, left, right, centre;
	top = average2(elevationMap[a], elevationMap[b]);
	left = average2(elevationMap[a], elevationMap[c]);
	right = average2(elevationMap[b], elevationMap[d]);
	bottom = average2(elevationMap[c], elevationMap[d]);

	elevationMap[(a + b) / 2] = top + (((double)rand() / (RAND_MAX))/2);
	elevationMap[(a + c) / 2] = left + (((double)rand() / (RAND_MAX)) / 2);
	elevationMap[(b + d) / 2] = right + (((double)rand() / (RAND_MAX)) / 2);
	elevationMap[(c + d) / 2] = bottom + (((double)rand() / (RAND_MAX)) / 2);

	centre = average4(elevationMap[(a + b) / 2], elevationMap[(a + c) / 2], elevationMap[(b + d) / 2], elevationMap[(c + d) / 2]);
	elevationMap[(a + b + c + d) / 4] = centre + (((double)rand() / (RAND_MAX)) / 2);
}

bool InitWindow();

bool LoadTextures();
bool LoadShaders();
void read_user_input();
void CreateShapes();
void InitUniforms();
void InitInputAssembler();
void InitRasterizer();
void InitPerSampleProcessing();

void MainLoop();
void Render();
void UpdateRotation();
void Cleanup();
void processInput(GLFWwindow *window);


int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to load GLFW!" << std::endl;
		return 1;
	}

	if (!InitWindow())
	{
		std::cerr << "Failed to create window!" << std::endl;
		Cleanup();
		return 1;
	}

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to load GLEW!" << std::endl;
		Cleanup();
		return 1;
	}

	//init time measurement
	::QueryPerformanceFrequency(&clockFreq);
	::QueryPerformanceCounter(&prevTime);

	//init DevIL
	ilInit();

	if (!LoadTextures())
	{
		Cleanup();
		return 1;
	}

	if (!LoadShaders())
	{
		std::cerr << "Failed to load shaders!" << std::endl;
		Cleanup();
		return 1;
	}
	read_user_input();
	CreateShapes();
	InitUniforms();
	InitInputAssembler();
	InitRasterizer();
	InitPerSampleProcessing();

	MainLoop();

	Cleanup();

	return 0;
}

bool InitWindow()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(ClientAreaWidth, ClientAreaHeight, "Terrain Modelling", nullptr, nullptr);

	if (!window)
		return false;

	//set the window's OpenGL context as the current OpenGL context

	glfwMakeContextCurrent(window);

	//set event handlers for the window

	glfwSetKeyCallback(window, key_callback);

	//the parameter 1 means VSync is enabled
	//change to 0 to disable VSync

	glfwSwapInterval(1);

	return true;
}

bool LoadTextures()
{
	ILuint imageId;
	ilGenImages(1, &imageId);
	ilBindImage(imageId);

	if (!ilLoadImage("pic0048.gif"))
	{
		std::cerr << "Unable to open crate.gif" << std::endl;
		return false;
	}

	//this is a GIF image, which uses a palette,
	//so we need to convert it to RGBA
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	//create the texture and store data into it

	glGenTextures(1, &crateTexture);
	glBindTexture(GL_TEXTURE_2D, crateTexture); 
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

	ilDeleteImages(1, &imageId);

	//configure the sampling method for the texture

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	return true;
}

bool LoadShaders()
{
	//load shaders

	GLenum vertexShader, fragmentShader;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	bool success = false;

	if (try_compile_shader_from_file(vertexShader, "VertexShader.glsl") &&
		try_compile_shader_from_file(fragmentShader, "FragmentShader.glsl"))
	{
		gpuProgram = glCreateProgram();

		glAttachShader(gpuProgram, vertexShader);
		glAttachShader(gpuProgram, fragmentShader);

		success = try_link_program(gpuProgram);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return success;
}

void read_user_input()
{
	std::cout << "\n#Enter a value for the size of the vertex array (can take value 1 to 5 in formula (2^n+1))\n";
	std::cin >> user_input;
	grid_Size = pow(2, user_input) + 1;
	elevationMap = new float[grid_Size*grid_Size];
	for (int i = 0; i < grid_Size*grid_Size; i++)
	{
		elevationMap[i] = 0;
	}

	std::cout << "\n#Press 'R' to use different random elevation value\n";
	std::cout << "\n#Press 'N' to manually enter the new size of vertex array\n";
	std::cout << "\n#Press 'C' to toggle rotation\n";
	std::cout << "\n#Use keys 'A','S','D','W' to move around in the scene\n";

}



void CreateShapes()
{
	const float one_by_sqrt_3 = 1.0f / ::sqrtf(3);
		
		//assigning the user input to the size
		SIZEE = grid_Size;

	Vertex vertexBufferData[33*33];
	float width = (float)2.0f/(SIZEE-1);
	for (int i = 0; i < SIZEE; i++)
	{
		for (int j = 0; j < SIZEE; j++)
		{
			//initializing vertices
			vertexBufferData[i*SIZEE + j].position[0] = -1.0f + (float)j*width;
			vertexBufferData[i*SIZEE + j].position[1] = ((double)rand() / (RAND_MAX)) - 1;
			vertexBufferData[i*SIZEE + j].position[2] = 1.0f - (float)i*width;

			//std::cout << "(" << i*SIZEE + j << ")" << vertexBufferData[i*SIZEE + j].position[0] << "," << vertexBufferData[i*SIZEE + j].position[1] << "," << vertexBufferData[i*SIZEE + j].position[2] << "  ";

			//initializing normals
			if (vertexBufferData[i*SIZEE + j].position[0] < 0)
				vertexBufferData[i*SIZEE + j].normal[0] = -one_by_sqrt_3;
			else
				vertexBufferData[i*SIZEE + j].normal[0] = +one_by_sqrt_3;

			vertexBufferData[i*SIZEE + j].normal[1] = +one_by_sqrt_3;

			if (vertexBufferData[i*SIZEE + j].position[2] < 0)
				vertexBufferData[i*SIZEE + j].normal[2] = -one_by_sqrt_3;
			else
				vertexBufferData[i*SIZEE + j].normal[2] = +one_by_sqrt_3;

			//initializing texture coordinates
			if (vertexBufferData[i*SIZEE + j].position[0] < 0)
				vertexBufferData[i*SIZEE + j].texcoord[0] = 0.0f;
			else
				vertexBufferData[i*SIZEE + j].texcoord[0] = 1.0f;

			if (vertexBufferData[i*SIZEE + j].position[2] < 0)
				vertexBufferData[i*SIZEE + j].texcoord[1] = 0.0f;
			else
				vertexBufferData[i*SIZEE + j].texcoord[1] = +1.0f;

		}
		//std::cout << std::endl;
	}

	//assigning the y coordinates of the vertices to the elevation map
	for (int i = 0; i < SIZEE*SIZEE; i++)
	{
		elevationMap[i] = vertexBufferData[i].position[1];
	}

	//the number of passes of mid point displacement that will cover all the vertices 
	int num_Passes = log2(SIZEE - 1);
	
	//variables
	int i = 0;
	int c = 0;
	int j = SIZEE - 1;
	int increment = SIZEE * (SIZEE-1);
	
	//stores the index of the corner points of every square
	int index1, index2, index3, index4;
	
	while (i<num_Passes)
	{	
		//pow(2, (i + i)) is the number of iterations in each pass
		for (int k = 1; k <= pow(2, (i + i)); k++)
		{
			index1 = c;
			index2 = index1 + j;
			if (k == 1)
			{
				index3 = index2 * SIZEE;
				increment = index3 - index2;
			}
			else
				index3 = index2 + increment;
			index4 = index3 + j;

			//passing the indexes to the function to fill up the elevation map
			midPointDisplacement(index1, index2, index3, index4);

			//std::cout << index1 << " " << index2 << " " << index3 << " " << index4 << " " << "iteration " << k << "pass " << i << std::endl;
			
			if (i != 0)
			{
				int m = pow(2, i);
				if (k % m == 0)
					c = index4 - SIZEE + 1;
				else
					c += j;
			}
		}
		j /= 2;
		i++;
		c = 0;
	}

	//normalizing the elevation map to 0 and 1
	for (int i = 0; i < SIZEE*SIZEE; i++)
	{	
		
		//std::cout<< "previous value " << vertexBufferData[i].position[1] << " new value " << elevationMap[i] << std::endl;
		if (elevationMap[i] < 0)
			elevationMap[i] = 0;
		if (elevationMap[i] > 1)
			elevationMap[i] = 1;
		vertexBufferData[i].position[1] = elevationMap[i];
	}

	//creating index buffer
	unsigned short indexBufferData[(33 - 1)*(33 - 1) * 6];
	
	//generalized formula for storing indices of vertices in the way they are supposed to be rendered (works for any number of passes)
	int n = 0;
	for (int i = 1; i < SIZEE*SIZEE-SIZEE; i++)
	{
		if (i%SIZEE == 0 && i > 0)
		{
			continue;
		}
		indexBufferData[n] = i;
		indexBufferData[n + 1] = i+ SIZEE;
		indexBufferData[n + 2] = i + SIZEE - 1;
		indexBufferData[n + 3] = i + SIZEE - 1;
		indexBufferData[n + 4] = i - 1;
		indexBufferData[n + 5] = i;
		n += 6;
	}
	

	//create vertex buffer and store data into it

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, 0);

	//create index buffer and store data into it

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexBufferData), indexBufferData, 0);
}

void InitUniforms()
{
	locMatWorld = glGetUniformLocation(gpuProgram, "matWorld");
	locMatView = glGetUniformLocation(gpuProgram, "matView");
	locMatProjection = glGetUniformLocation(gpuProgram, "matProjection");
	locUseTexture = glGetUniformLocation(gpuProgram, "useTexture");
	locLightDirection = glGetUniformLocation(gpuProgram, "lightDirection");
	locTex = glGetUniformLocation(gpuProgram, "tex");

	glProgramUniformMatrix4fv(gpuProgram, locMatWorld, 1, GL_FALSE, glm::value_ptr(glm::mat4()));
	glProgramUniform1i(gpuProgram, locUseTexture, use_texture);

	glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
	glProgramUniform3fv(gpuProgram, locLightDirection, 1, glm::value_ptr(lightDir));

	

	glm::mat4 matView = glm::lookAtRH(eyePosition, lookAt, upDirection);
	glProgramUniformMatrix4fv(gpuProgram, locMatView, 1, GL_FALSE, glm::value_ptr(matView));

	const float fieldOfView = glm::quarter_pi<float>();
	const float nearPlane = 0.01f;
	const float farPlane = 10.0f;

	glm::mat4 matProjection = glm::perspectiveFovRH(fieldOfView, (float)ClientAreaWidth, (float)ClientAreaHeight, nearPlane, farPlane);
	glProgramUniformMatrix4fv(gpuProgram, locMatProjection, 1, GL_FALSE, glm::value_ptr(matProjection));

	glProgramUniform1i(gpuProgram, locTex, 0);//use texture slot 0
}

void InitInputAssembler()
{
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	//each vertex is 3 floats for the position, 3 floats for the normal and 2 floats for the texture coordinates

					//index		num_components	type		normalize?	stride			offset
	glVertexAttribPointer(0,	3,				GL_FLOAT,	GL_FALSE,	sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

					//index		num_components	type		normalize?	stride			offset
	glVertexAttribPointer(1,	3,				GL_FLOAT,	GL_FALSE,	sizeof(Vertex), (void*) sizeof(float[3]));
	glEnableVertexAttribArray(1);

					//index		num_components	type		normalize?	stride			offset
	glVertexAttribPointer(2,	2,				GL_FLOAT,	GL_FALSE,	sizeof(Vertex), (void*) sizeof(float[6]));
	glEnableVertexAttribArray(2);
}

void InitRasterizer()
{
	glViewport(0, 0, ClientAreaWidth, ClientAreaHeight);
	//glEnable(GL_CULL_FACE);
}

void InitPerSampleProcessing()
{
	glClearColor(0.6f, 0.7f, 0.9f, 1.0f);
	glEnable(GL_DEPTH_TEST);
}

void Cleanup()
{
	glDeleteProgram(gpuProgram);
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &indexBuffer);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteTextures(1, &crateTexture);

	ilShutDown();
	glfwTerminate();
}

void Render()
{
	processInput(window);
	if(flag)
		InitUniforms();

	//clear the depth and color buffers

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//bind whatever needs to be bound

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBindVertexArray(vertexArray);
	glUseProgram(gpuProgram);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, crateTexture);

	//draw the triangles
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, (SIZEE - 1)*(SIZEE - 1) * 6, GL_UNSIGNED_SHORT, (void*)0);

	//swap the back and front buffers

	glfwSwapBuffers(window);

	UpdateRotation();
}

//to move around in the scene
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5 * 0.05;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		eyePosition += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		eyePosition -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		eyePosition -= glm::normalize(glm::cross(cameraFront, upDirection)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		eyePosition += glm::normalize(glm::cross(cameraFront, upDirection)) * cameraSpeed;
}

void UpdateRotation()
{
	static const float angularSpeed = 0.5f;
	static const glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

	LARGE_INTEGER currentTime;
	::QueryPerformanceCounter(&currentTime);

	float elapsedTime = (float)(double(currentTime.QuadPart - prevTime.QuadPart) / clockFreq.QuadPart);
	angle += angularSpeed * elapsedTime;

	while (angle > glm::two_pi<float>())
		angle -= glm::two_pi<float>();

	glm::mat4 matWorld;
	matWorld = glm::rotate(matWorld, angle, rotationAxis);

	glProgramUniformMatrix4fv(gpuProgram, locMatWorld, 1, GL_FALSE, glm::value_ptr(matWorld));

	prevTime = currentTime;
}

void MainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		Render();
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		else if (key == GLFW_KEY_T)
		{
			use_texture = !use_texture;
			glProgramUniform1i(gpuProgram, locUseTexture, use_texture);
		}
		else if (key == GLFW_KEY_N)  //to consider user input at any point during the run time
		{
			read_user_input();
			CreateShapes();
			InitUniforms();
			InitInputAssembler();
			InitRasterizer();
			InitPerSampleProcessing();
		}
		else if (key == GLFW_KEY_R) //to update the random values assigned to elevation map to get different elevation map
		{
			CreateShapes();
			InitUniforms();
			InitInputAssembler();
			InitRasterizer();
			InitPerSampleProcessing();
		}
		else if (key == GLFW_KEY_C) //to toggle the rotation
		{
			flag = !flag;
		}
	}
}
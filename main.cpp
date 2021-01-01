#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stddef.h> /*for function: offsetof */
#include <math.h>
#include <string.h>
#include "../GL/glew.h"
#include "../GL/glut.h"
#include "../shader_lib/shader.h"
#include "glm/glm.h"
extern "C"
{
	#include "glm_helper.h"
}

struct Vertex
{
	GLfloat position[3];
	GLfloat normal[3];
	GLfloat texcoord[2];
};
typedef struct Vertex Vertex;

void init(void);
void display(void);
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void keyboardup(unsigned char key, int x, int y);
void motion(int x, int y);
void mouse(int button, int state, int x, int y);
void idle(void);
void draw_light_bulb(void);
void camera_light_ball_move();
char* readShader(char*);
void shaderInit(char*, char*);
void drawObject();
void drawDepthMap();
GLuint loadTexture(char* name, GLfloat width, GLfloat height);

namespace
{
	char *obj_file_dir = "../Resources/Ball.obj";
	char *obj_file_dir2 = "../Resources/bunny.obj";
	char *main_tex_dir = "../Resources/Stone.ppm";
	char *floor_tex_dir = "../Resources/WoodFine.ppm";
	char *plane_file_dir = "../Resources/Plane.obj";
	char *noise_tex_dir = "../Resources/noise.ppm";
	
	GLfloat light_rad = 0.05; //radius of the light bulb
	float eyet = -5.59; //theta in degree
	float eyep = 83.2; //phi in degree
	bool mleft = false;
	bool mright = false;
	bool mmiddle = false;
	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;
	bool lforward = false;
	bool lbackward = false;
	bool lleft = false;
	bool lright = false;
	bool lup = false;
	bool ldown = false;
	bool bforward = false;
	bool bbackward = false;
	bool bleft = false;
	bool bright = false;
	bool bup = false;
	bool bdown = false;
	bool bx = false;
	bool by = false;
	bool bz = false;
	bool brx = false;
	bool bry = false;
	bool brz = false;

	int mousex = 0;
	int mousey = 0;
}

const float speed = 0.03; // camera / light / ball moving speed
const float rotation_speed = 0.05; // ball rotating speed

GLuint mainTextureID;
GLuint floorTextureID;
GLuint noiseTextureID;

GLMmodel *model;
GLMmodel *planeModel;
GLMmodel *subModel;

GLuint bunnyVBO;
GLuint ballVBO;
GLuint planeVBO;

GLuint bunnyVaoHandle;
GLuint ballVaoHandle;
GLuint planeVaoHandle;

float eyex = -3.291;
float eyey = 1.57;
float eyez = 11.89;

GLfloat light_pos[] = { 1.1, 3.5, 1.3 };
GLfloat ball_pos[] = { 0.0, 0.0, 0.0 };
GLfloat ball_rot[] = { 0.0, 0.0, 0.0 };
GLfloat plane_pos[] = { 0.0, -5.0, 0.0 };
GLfloat plane_rot[] = { 0.0, 0.0, 0.0 };
GLfloat subModel_pos[] = { -2.295, -5.0, -2.0 };
GLfloat subModel_rot[] = { 0.0, 0.0, 0.0 };

int shaderProgram;
const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
GLuint depthMapFBO;
GLuint depthMap;
float dissolveThres = 0.0;

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	
	// remember to replace "YourStudentID" with your own student ID
	glutCreateWindow("Demo");
	glutReshapeWindow(512, 512);

	glewInit();

	init();
	shaderInit("../Shaders/shadow.vert", "../Shaders/shadow.frag");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();

	glmDelete(model);
	return 0;
}

void init(void)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glEnable(GL_CULL_FACE);

	mainTextureID = loadTexture(main_tex_dir, 1024, 1024);
	floorTextureID = loadTexture(floor_tex_dir, 512, 512);
	noiseTextureID = loadTexture(noise_tex_dir, 320, 320);

	model = glmReadOBJ(obj_file_dir);
	glmUnitize(model);
	glmFacetNormals(model);
	glmVertexNormals(model, 90.0, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	print_model_info(model);

	planeModel = glmReadOBJ(plane_file_dir);
	glmFacetNormals(planeModel);
	glmVertexNormals(planeModel, 90.0, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	print_model_info(planeModel);	

	subModel = glmReadOBJ(obj_file_dir2);
	glmFacetNormals(subModel);
	glmVertexNormals(subModel, 90.0, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	print_model_info(subModel);

	const int bunnyVertexCount = subModel->numvertices;
	int bunnyTrianglesCount = subModel->numtriangles;

	const int ballVertexCount = model->numvertices;
	int ballTrianglesCount = model->numtriangles;

	const int planeVertexCount = planeModel->numvertices;
	int planeTrianglesCount = planeModel->numtriangles;

	Vertex* bunnyVertex = new Vertex[bunnyTrianglesCount * 3];
	GLMtriangle* bunnyTriangles = subModel->triangles;

	Vertex* ballVertex = new Vertex[ballTrianglesCount * 3];
	GLMtriangle* ballTriangles = model->triangles;

	Vertex* planeVertex = new Vertex[planeTrianglesCount * 3];
	GLMtriangle* planeTriangles = planeModel->triangles;
	
	//=============================================================================================

	for (int i = 0; i < bunnyTrianglesCount; i++) {
		int vindex[3] = { subModel->triangles[i].vindices[0], subModel->triangles[i].vindices[1], subModel->triangles[i].vindices[2] };
		int nindex[3] = { subModel->triangles[i].nindices[0], subModel->triangles[i].nindices[1], subModel->triangles[i].nindices[2] };
		int tindex[3] = { subModel->triangles[i].tindices[0], subModel->triangles[i].tindices[1], subModel->triangles[i].tindices[2] };


		for (int j = 0; j < 3; j++) {
			//Position
			bunnyVertex[i * 3 + j].position[0] = subModel->vertices[vindex[j] * 3 + 0];		//X軸座標
			bunnyVertex[i * 3 + j].position[1] = subModel->vertices[vindex[j] * 3 + 1];		//Y軸座標
			bunnyVertex[i * 3 + j].position[2] = subModel->vertices[vindex[j] * 3 + 2];		//Z軸座標

			//Normal
			bunnyVertex[i * 3 + j].normal[0] = subModel->normals[nindex[j] * 3 + 0];
			bunnyVertex[i * 3 + j].normal[1] = subModel->normals[nindex[j] * 3 + 1];
			bunnyVertex[i * 3 + j].normal[2] = subModel->normals[nindex[j] * 3 + 2];

			//TextureCoord
			bunnyVertex[i * 3 + j].texcoord[0] = subModel->texcoords[tindex[j] * 2 + 0];
			bunnyVertex[i * 3 + j].texcoord[1] = subModel->texcoords[tindex[j] * 2 + 1];
		}
	}

	// Create VAO
	glGenVertexArrays(1, &bunnyVaoHandle);
	glBindVertexArray(bunnyVaoHandle);

	// Create VBOs
	glGenBuffers(1, &bunnyVBO);
	glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * bunnyTrianglesCount * 3, bunnyVertex, GL_STATIC_DRAW);

	// Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); // stride 0 for tightly packed

	// Normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal))); // stride 0 for tightly packed

	// Texture coordinates
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texcoord))); // stride 0 for tightly packed

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//=============================================================================================

	for (int i = 0; i < ballTrianglesCount; i++) {
		int vindex[3] = { model->triangles[i].vindices[0], model->triangles[i].vindices[1], model->triangles[i].vindices[2] };
		int nindex[3] = { model->triangles[i].nindices[0], model->triangles[i].nindices[1], model->triangles[i].nindices[2] };
		int tindex[3] = { model->triangles[i].tindices[0], model->triangles[i].tindices[1], model->triangles[i].tindices[2] };

		for (int j = 0; j < 3; j++) {
			//Position
			ballVertex[i * 3 + j].position[0] = model->vertices[vindex[j] * 3 + 0];		//X軸座標
			ballVertex[i * 3 + j].position[1] = model->vertices[vindex[j] * 3 + 1];		//Y軸座標
			ballVertex[i * 3 + j].position[2] = model->vertices[vindex[j] * 3 + 2];		//Z軸座標

			//Normal
			ballVertex[i * 3 + j].normal[0] = model->normals[nindex[j] * 3 + 0];
			ballVertex[i * 3 + j].normal[1] = model->normals[nindex[j] * 3 + 1];
			ballVertex[i * 3 + j].normal[2] = model->normals[nindex[j] * 3 + 2];

			//TextureCoord
			ballVertex[i * 3 + j].texcoord[0] = model->texcoords[tindex[j] * 2 + 0];
			ballVertex[i * 3 + j].texcoord[1] = model->texcoords[tindex[j] * 2 + 1];
		}
	}

	glGenVertexArrays(1, &ballVaoHandle);
	glBindVertexArray(ballVaoHandle);

	glGenBuffers(1, &ballVBO);
	glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * ballTrianglesCount * 3, ballVertex, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); // stride 0 for tightly packed

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal))); // stride 0 for tightly packed

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texcoord))); // stride 0 for tightly packed

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//=============================================================================================

	for (int i = 0; i < planeTrianglesCount; i++) {
		int vindex[3] = { planeModel->triangles[i].vindices[0], planeModel->triangles[i].vindices[1], planeModel->triangles[i].vindices[2] };
		int nindex[3] = { planeModel->triangles[i].nindices[0], planeModel->triangles[i].nindices[1], planeModel->triangles[i].nindices[2] };
		int tindex[3] = { planeModel->triangles[i].tindices[0], planeModel->triangles[i].tindices[1], planeModel->triangles[i].tindices[2] };

		for (int j = 0; j < 3; j++) {
			//Position
			planeVertex[i * 3 + j].position[0] = planeModel->vertices[vindex[j] * 3 + 0];		//X軸座標
			planeVertex[i * 3 + j].position[1] = planeModel->vertices[vindex[j] * 3 + 1];		//Y軸座標
			planeVertex[i * 3 + j].position[2] = planeModel->vertices[vindex[j] * 3 + 2];		//Z軸座標

			//Normal
			planeVertex[i * 3 + j].normal[0] = planeModel->normals[nindex[j] * 3 + 0];
			planeVertex[i * 3 + j].normal[1] = planeModel->normals[nindex[j] * 3 + 1];
			planeVertex[i * 3 + j].normal[2] = planeModel->normals[nindex[j] * 3 + 2];

			//TextureCoord
			planeVertex[i * 3 + j].texcoord[0] = planeModel->texcoords[tindex[j] * 2 + 0];
			planeVertex[i * 3 + j].texcoord[1] = planeModel->texcoords[tindex[j] * 2 + 1];
		}
	}
	glGenVertexArrays(1, &planeVaoHandle);
	glBindVertexArray(planeVaoHandle);

	glGenBuffers(1, &planeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * planeTrianglesCount * 3, planeVertex, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); // stride 0 for tightly packed

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal))); // stride 0 for tightly packed

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texcoord))); // stride 0 for tightly packed

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//=============================================================================================

	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);		//A framebuffer object is not complete without a color buffer so we need to explicitly tell OpenGL we're not going to render any color data.
	glReadBuffer(GL_NONE);		//A framebuffer object however is not complete without a color buffer so we need to explicitly tell OpenGL we're not going to render any color data.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//=============================================================================================

	delete[] ballVertex;
	delete[] bunnyVertex;
	delete[] planeVertex;
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	// 1. first render to depth map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawDepthMap();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. then render scene as normal with shadow mapping (using depth map)
	glViewport(0, 0, (GLfloat)glutGet(GLUT_WINDOW_WIDTH), (GLfloat)glutGet(GLUT_WINDOW_HEIGHT));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawObject();

	glutSwapBuffers();
	camera_light_ball_move();
}

void shaderInit(char* vShader, char* fShader) {
	int  Success;
	char infoLog[512];
	unsigned int vertexShader;
	unsigned int fragmentShader;

	char* vertexShaderSource = readShader(vShader);
	char* fragmentShaderSource = readShader(fShader);

	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check the correctness of VertexShader Compilation
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//Check the correctness of FragmentShader Compilation
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

char* readShader(char* shaderPath) {
	FILE *fp;
	fp = fopen(shaderPath, "r");
	char *buffer = (char*)malloc(sizeof(char) * 4096);
	char *data = (char*)malloc(sizeof(char) * 4096);
	buffer[0] = '\0';
	data[0] = '\0';

	if (fp == NULL) {
		std::cout << "Error" << std::endl;
	}

	while (fgets(buffer, 4096, fp) != NULL) {
		strcat(data, buffer);
	}
	free(buffer);
	fclose(fp);

	return data;
}

// please implement mode increase/decrease dissolve threshold in case '-' and case '=' (lowercase)
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
	{	// ESC
		break;
	}
	case '-': // increase dissolve threshold
	{
		dissolveThres += 0.1;
		if (dissolveThres >= 1.0) {
			dissolveThres = 1.0;
		}
		break;
	}
	case '=': // decrease dissolve threshold
	{
		dissolveThres -= 0.1;
		if (dissolveThres <= 0.0) {
			dissolveThres = 0.0;
		}
		break;
	}
	case 'd':
	{
		right = true;
		break;
	}
	case 'a':
	{
		left = true;
		break;
	}
	case 'w':
	{
		forward = true;
		break;
	}
	case 's':
	{
		backward = true;
		break;
	}
	case 'q':
	{
		up = true;
		break;
	}
	case 'e':
	{
		down = true;
		break;
	}
	case 't':
	{
		lforward = true;
		break;
	}
	case 'g':
	{
		lbackward = true;
		break;
	}
	case 'h':
	{
		lright = true;
		break;
	}
	case 'f':
	{
		lleft = true;
		break;
	}
	case 'r':
	{
		lup = true;
		break;
	}
	case 'y':
	{
		ldown = true;
		break;
	}
	case 'i':
	{
		bforward = true;
		break;
	}
	case 'k':
	{
		bbackward = true;
		break;
	}
	case 'l':
	{
		bright = true;
		break;
	}
	case 'j':
	{
		bleft = true;
		break;
	}
	case 'u':
	{
		bup = true;
		break;
	}
	case 'o':
	{
		bdown = true;
		break;
	}
	case '7':
	{
		bx = true;
		break;
	}
	case '8':
	{
		by = true;
		break;
	}
	case '9':
	{
		bz = true;
		break;
	}
	case '4':
	{
		brx = true;
		break;
	}
	case '5':
	{
		bry = true;
		break;
	}
	case '6':
	{
		brz = true;
		break;
	}

	//special function key
	case 'z'://move light source to front of camera
	{
		light_pos[0] = eyex + cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180);
		light_pos[1] = eyey + sin(eyet*M_PI / 180);
		light_pos[2] = eyez - cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180);
		break;
	}
	case 'x'://move ball to front of camera
	{
		ball_pos[0] = eyex + cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180) * 3;
		ball_pos[1] = eyey + sin(eyet*M_PI / 180) * 5;
		ball_pos[2] = eyez - cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180) * 3;
		break;
	}
	case 'c'://reset all pose
	{
		light_pos[0] = 1.1;
		light_pos[1] = 3.5;
		light_pos[2] = 1.3;
		ball_pos[0] = 0;
		ball_pos[1] = 0;
		ball_pos[2] = 0;
		ball_rot[0] = 0;
		ball_rot[1] = 0;
		ball_rot[2] = 0;
		eyex = -3.291;
		eyey = 1.57;
		eyez = 11.89;
		eyet = -5.59; //theta in degree
		eyep = 83.2; //phi in degree
		break;
	}
	default:
	{
		break;
	}
	}
}

//no need to modify the following functions
void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.001f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
}

void motion(int x, int y)
{
	if (mleft)
	{
		eyep -= (x-mousex)*0.1;
		eyet -= (y - mousey)*0.12;
		if (eyet > 89.9)
			eyet = 89.9;
		else if (eyet < -89.9)
			eyet = -89.9;
		if (eyep > 360)
			eyep -= 360;
		else if (eyep < 0)
			eyep += 360;
	}
	mousex = x;
	mousey = y;
}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if(state == GLUT_DOWN && !mright && !mmiddle)
		{
			mleft = true;
			mousex = x;
			mousey = y;
		}
		else
			mleft = false;
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN && !mleft && !mmiddle)
		{
			mright = true;
			mousex = x;
			mousey = y;
		}
		else
			mright = false;
	}
	else if (button == GLUT_MIDDLE_BUTTON)
	{
		if (state == GLUT_DOWN && !mleft && !mright)
		{
			mmiddle = true;
			mousex = x;
			mousey = y;
		}
		else
			mmiddle = false;
	}
}

void camera_light_ball_move()
{
	GLfloat dx = 0, dy = 0, dz=0;
	if(left|| right || forward || backward || up || down)
	{ 
		if (left)
			dx = -speed;
		else if (right)
			dx = speed;
		if (forward)
			dy = speed;
		else if (backward)
			dy = -speed;
		eyex += dy*cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180) + dx*sin(eyep*M_PI / 180);
		eyey += dy*sin(eyet*M_PI / 180);
		eyez += dy*(-cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180)) + dx*cos(eyep*M_PI / 180);
		if (up)
			eyey += speed;
		else if (down)
			eyey -= speed;
	}
	if(lleft || lright || lforward || lbackward || lup || ldown)
	{
		dx = 0;
		dy = 0;
		if (lleft)
			dx = -speed;
		else if (lright)
			dx = speed;
		if (lforward)
			dy = speed;
		else if (lbackward)
			dy = -speed;
		light_pos[0] += dy*cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180) + dx*sin(eyep*M_PI / 180);
		light_pos[1] += dy*sin(eyet*M_PI / 180);
		light_pos[2] += dy*(-cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180)) + dx*cos(eyep*M_PI / 180);
		if (lup)
			light_pos[1] += speed;
		else if(ldown)
			light_pos[1] -= speed;
	}
	if (bleft || bright || bforward || bbackward || bup || bdown)
	{
		dx = 0;
		dy = 0;
		if (bleft)
			dx = -speed;
		else if (bright)
			dx = speed;
		if (bforward)
			dy = speed;
		else if (bbackward)
			dy = -speed;
		ball_pos[0] += dy*cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180) + dx*sin(eyep*M_PI / 180);
		ball_pos[1] += dy*sin(eyet*M_PI / 180);
		ball_pos[2] += dy*(-cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180)) + dx*cos(eyep*M_PI / 180);
		if (bup)
			ball_pos[1] += speed;
		else if (bdown)
			ball_pos[1] -= speed;
	}
	if(bx||by||bz || brx || bry || brz)
	{
		dx = 0;
		dy = 0;
		dz = 0;
		if (bx)
			dx = -rotation_speed;
		else if (brx)
			dx = rotation_speed;
		if (by)
			dy = rotation_speed;
		else if (bry)
			dy = -rotation_speed;
		if (bz)
			dz = rotation_speed;
		else if (brz)
			dz = -rotation_speed;
		ball_rot[0] += dx;
		ball_rot[1] += dy;
		ball_rot[2] += dz;
	}
}

void draw_light_bulb()
{
	GLUquadric *quad;
	quad = gluNewQuadric();
	glPushMatrix();
	glColor3f(0.4, 0.5, 0);
	glTranslatef(light_pos[0], light_pos[1], light_pos[2]);
	gluSphere(quad, light_rad, 40, 20);
	glPopMatrix();
}

void keyboardup(unsigned char key, int x, int y)
{
	switch (key) {
	case 'd':
	{
		right =false;
		break;
	}
	case 'a':
	{
		left = false;
		break;
	}
	case 'w':
	{
		forward = false;
		break;
	}
	case 's':
	{
		backward = false;
		break;
	}
	case 'q':
	{
		up = false;
		break;
	}
	case 'e':
	{
		down = false;
		break;
	}
	case 't':
	{
		lforward = false;
		break;
	}
	case 'g':
	{
		lbackward = false;
		break;
	}
	case 'h':
	{
		lright = false;
		break;
	}
	case 'f':
	{
		lleft = false;
		break;
	}
	case 'r':
	{
		lup = false;
		break;
	}
	case 'y':
	{
		ldown = false;
		break;
	}
	case 'i':
	{
		bforward = false;
		break;
	}
	case 'k':
	{
		bbackward = false;
		break;
	}
	case 'l':
	{
		bright = false;
		break;
	}
	case 'j':
	{
		bleft = false;
		break;
	}
	case 'u':
	{
		bup = false;
		break;
	}
	case 'o':
	{
		bdown = false;
		break;
	}
	case '7':
	{
		bx = false;
		break;
	}
	case '8':
	{
		by = false;
		break;
	}
	case '9':
	{
		bz = false;
		break;
	}
	case '4':
	{
		brx = false;
		break;
	}
	case '5':
	{
		bry = false;
		break;
	}
	case '6':
	{
		brz = false;
		break;
	}

	default:
	{
		break;
	}
	}
}

void idle(void)
{
	subModel_rot[1] += 1;
	glutPostRedisplay();
}

GLuint loadTexture(char* name, GLfloat width, GLfloat height)
{
	return glmLoadTexture(name, false, true, true, true, &width, &height);
}

void drawDepthMap()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 0.001f, 100.0f);

	float Projection[16];
	glGetFloatv(GL_PROJECTION_MATRIX, Projection);

	glUseProgram(shaderProgram);

	GLint P = glGetUniformLocation(shaderProgram, "Projection");
	glUniformMatrix4fv(P, 1, GL_FALSE, Projection);

	glMatrixMode(GL_MODELVIEW);																//操作MV
	glLoadIdentity();
	gluLookAt(
		light_pos[0],
		light_pos[1],
		light_pos[2],
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f);

	float ViewMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, ViewMatrix);

	glUseProgram(shaderProgram);

	GLint V = glGetUniformLocation(shaderProgram, "ViewMatrix");
	glUniformMatrix4fv(V, 1, GL_FALSE, ViewMatrix);

	glLoadIdentity();																		// Ball's Model Matrix based on light
		glTranslatef(ball_pos[0], ball_pos[1], ball_pos[2]);
		glRotatef(ball_rot[0], 1, 0, 0);
		glRotatef(ball_rot[1], 0, 1, 0);
		glRotatef(ball_rot[2], 0, 0, 1);
		// glColor3f(1, 1, 1);

	float ModelMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);											// Ball's Model matrix based on light

	glUseProgram(shaderProgram);															// 綁定Shader Code

	GLint M = glGetUniformLocation(shaderProgram, "ModelMatrix");
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	GLint noise = glGetUniformLocation(shaderProgram, "noise");
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, noiseTextureID);
	glUniform1i(noise, 4);

	GLint DT = glGetUniformLocation(shaderProgram, "dissolveThres");						// Dissolve's Threshold
	glUniform1f(DT, dissolveThres);

	glBindVertexArray(ballVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, model->numtriangles * 3);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, NULL);

	//=========================================================================================================

	glLoadIdentity();																		// plane's Model matrix based on light
		glTranslatef(plane_pos[0], plane_pos[1], plane_pos[2]);
		glRotatef(plane_rot[0], 1, 0, 0);
		glRotatef(plane_rot[1], 0, 1, 0);
		glRotatef(plane_rot[2], 0, 0, 1);
		glColor3f(1, 1, 1);

	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);											// Plane's Model matrix based on light
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	DT = glGetUniformLocation(shaderProgram, "dissolveThres");								// Dissolve's Threshold
	glUniform1f(DT, 0);

	glBindVertexArray(planeVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, planeModel->numtriangles * 3);
	glBindVertexArray(0);

	//=========================================================================================================

	glLoadIdentity();																		// Bunny's Model Matrix based on light
		glTranslatef(subModel_pos[0], subModel_pos[1], subModel_pos[2]);
		glRotatef(subModel_rot[0], 1, 0, 0);
		glRotatef(subModel_rot[1], 0, 1, 0);
		glRotatef(subModel_rot[2], 0, 0, 1);

	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);											// Bunny's Model matrix based on light
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	DT = glGetUniformLocation(shaderProgram, "dissolveThres");								// Dissolve's Threshold
	glUniform1f(DT, 0);

	glBindVertexArray(bunnyVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, subModel->numtriangles * 3);
	glBindVertexArray(0);

	GLint light = glGetUniformLocation(shaderProgram, "lightPosition");
	glUniform3fv(light, 1, light_pos);

	GLint eyeX = glGetUniformLocation(shaderProgram, "eyeX");
	glUniform1f(eyeX, eyex);

	GLint eyeY = glGetUniformLocation(shaderProgram, "eyeY");
	glUniform1f(eyeY, eyey);

	GLint eyeZ = glGetUniformLocation(shaderProgram, "eyeZ");
	glUniform1f(eyeZ, eyez);

}

void drawObject()
{
	//Get Projection Matrix
	//===========================================================================

	glBindTexture(GL_TEXTURE_2D, depthMap);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)glutGet(GLUT_WINDOW_WIDTH) / (GLfloat)glutGet(GLUT_WINDOW_HEIGHT) , 0.001f, 100.0f);
	glPushMatrix();

	float Projection[16];
	glGetFloatv(GL_PROJECTION_MATRIX, Projection);									// Projection matrix
	
	glUseProgram(shaderProgram);

	GLint P = glGetUniformLocation(shaderProgram, "Projection");
	glUniformMatrix4fv(P, 1, GL_FALSE, Projection);

	//=============================================================================

	glLoadIdentity();
	glOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 0.001f, 100.0f);

	float lightProjection[16];
	glGetFloatv(GL_PROJECTION_MATRIX, lightProjection);

	glUseProgram(shaderProgram);

	GLint LP = glGetUniformLocation(shaderProgram, "lightProjection");
	glUniformMatrix4fv(LP, 1, GL_FALSE, lightProjection);

	glPopMatrix();																		// Perspective Projection

	//=============================================================================

	glMatrixMode(GL_MODELVIEW);															// 操作MV
	glLoadIdentity();
	gluLookAt(																			// View Matrix
		eyex,
		eyey,
		eyez,
		eyex + cos(eyet*M_PI / 180)*cos(eyep*M_PI / 180),
		eyey + sin(eyet*M_PI / 180),
		eyez - cos(eyet*M_PI / 180)*sin(eyep*M_PI / 180),
		0.0,
		1.0,
		0.0);

	//glPushMatrix();
		glUseProgram(NULL);
		
		for (int i = 0; i < 5; ++i) {
			glActiveTexture(GL_TEXTURE0 + i);
			glDisable(GL_TEXTURE_2D);
		}
		glColor3f(1, 1, 1);
		draw_light_bulb();
		for (int i = 0; i < 5; ++i) {
			glActiveTexture(GL_TEXTURE0 + i);
			glEnable(GL_TEXTURE_2D);
		}
	//glPopMatrix();

	float ViewMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, ViewMatrix);										// Ball's MV matrix

	glUseProgram(shaderProgram);														// 綁定Shader Code

	GLint V = glGetUniformLocation(shaderProgram, "ViewMatrix");
	glUniformMatrix4fv(V, 1, GL_FALSE, ViewMatrix);

	//=============================================================================

	glLoadIdentity();
	gluLookAt(
		light_pos[0],
		light_pos[1],
		light_pos[2],
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f);

	float lightViewMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, lightViewMatrix);

	glUseProgram(shaderProgram);

	GLint LV = glGetUniformLocation(shaderProgram, "lightViewMatrix");
	glUniformMatrix4fv(LV, 1, GL_FALSE, lightViewMatrix);

	//=============================================================================

	glLoadIdentity();
		glTranslatef(ball_pos[0], ball_pos[1], ball_pos[2]);
		glRotatef(ball_rot[0], 1, 0, 0);
		glRotatef(ball_rot[1], 0, 1, 0);
		glRotatef(ball_rot[2], 0, 0, 1);
		glColor3f(1, 1, 1);

	float ModelMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);										// Ball's Model matrix

	glUseProgram(shaderProgram);														// 綁定Shader Code

	GLint M = glGetUniformLocation(shaderProgram, "ModelMatrix");
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	//=============================================================================

	GLint tex = glGetUniformLocation(shaderProgram, "myTexture");
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, mainTextureID);
	glUniform1i(tex, 0);

	GLint noise = glGetUniformLocation(shaderProgram, "noise");
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, noiseTextureID);
	glUniform1i(noise, 4);

	GLint DT = glGetUniformLocation(shaderProgram, "dissolveThres");					// Dissolve's Threshold
	glUniform1f(DT, dissolveThres);

	glBindVertexArray(ballVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, model->numtriangles * 3);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, NULL);

	//=========================================================================================================

	glLoadIdentity();																		
		glTranslatef(plane_pos[0], plane_pos[1], plane_pos[2]);							// plane's Model matrix
		glRotatef(plane_rot[0], 1, 0, 0);
		glRotatef(plane_rot[1], 0, 1, 0);
		glRotatef(plane_rot[2], 0, 0, 1);
		glColor3f(1, 1, 1);

	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);										// Plane's Model matrix
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	tex = glGetUniformLocation(shaderProgram, "myTexture");
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, floorTextureID);
	glUniform1i(tex, 1);

	DT = glGetUniformLocation(shaderProgram, "dissolveThres");							// Dissolve's Threshold
	glUniform1f(DT, 0);

	glBindVertexArray(planeVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, planeModel->numtriangles * 3);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, NULL);

	//=========================================================================================================

	glLoadIdentity();																	// Bunny's Model Matrix
		glTranslatef(subModel_pos[0], subModel_pos[1], subModel_pos[2]);
		glRotatef(subModel_rot[0], 1, 0, 0);
		glRotatef(subModel_rot[1], 0, 1, 0);
		glRotatef(subModel_rot[2], 0, 0, 1);


	glGetFloatv(GL_MODELVIEW_MATRIX, ModelMatrix);										// Bunny's Model matrix
	glUseProgram(shaderProgram);
	glUniformMatrix4fv(M, 1, GL_FALSE, ModelMatrix);

	tex = glGetUniformLocation(shaderProgram, "myTexture");
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, mainTextureID);
	glUniform1i(tex, 2);

	DT = glGetUniformLocation(shaderProgram, "dissolveThres");							// Dissolve's Threshold
	glUniform1f(DT, 0);

	glBindVertexArray(bunnyVaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, subModel->numtriangles * 3);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, NULL);

	GLint light = glGetUniformLocation(shaderProgram, "lightPosition");
	glUniform3fv(light, 1, light_pos);

	GLint eyeX = glGetUniformLocation(shaderProgram, "eyeX");
	glUniform1f(eyeX, eyex);

	GLint eyeY = glGetUniformLocation(shaderProgram, "eyeY");
	glUniform1f(eyeY, eyey);

	GLint eyeZ = glGetUniformLocation(shaderProgram, "eyeZ");
	glUniform1f(eyeZ, eyez);

	GLint depth = glGetUniformLocation(shaderProgram, "depthMap");
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glUniform1i(depth, 3);
}
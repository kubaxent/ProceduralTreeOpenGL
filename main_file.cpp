#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include <algorithm>
#include <time.h>

using namespace glm;
using namespace std;

float my_time = 0.0f;
float speed_x=0;
float speed_y=0;
float speed_z=0;

float aspectRatio=1;

float fly_speed = 30.0f;

//Growth parameters
float branch_thresh = 5.0f;
int tree_levels = 5;
float max_angle = 1.0f;


vector<float> random_angles;
vector<int> random_branch_numbers;
ShaderProgram *sp;

//Texture handles
GLuint bark;
GLuint sky;
GLuint ground;
GLuint leaves;

vector< float > m_vertices;
vector< float > m_uvs;
vector< float > m_normals;
vector< float > m_colors;

vector<vec3> m_growth_vertices; //vertices from which branches can grow

vector< float > g_vertices;
vector< float > g_uvs;
vector< float > g_normals;
vector< float > g_colors;

vector< float > l_vertices;
vector< float > l_uvs;
vector< float > l_normals;
vector< float > l_colors;

bool loadOBJ(
	const char * path,
	std::vector<float> & out_vertices,
	std::vector<float> & out_uvs,
	std::vector<float> & out_normals,
	std::vector<float> & out_colors
){
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if( file == NULL ){
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while( 1 ){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF){
			break; // EOF = End Of File. Quit the loop.
		}

		// else : parse lineHeader

		if ( strcmp( lineHeader, "v" ) == 0 ){
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}else if ( strcmp( lineHeader, "vt" ) == 0 ){
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y );
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}else if ( strcmp( lineHeader, "vn" ) == 0 ){
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}else if ( strcmp( lineHeader, "f" ) == 0 ){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                        &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                        &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
			if (matches != 9){
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices    .push_back(uvIndex[0]);
			uvIndices    .push_back(uvIndex[1]);
			uvIndices    .push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for( unsigned int i=0; i<vertexIndices.size(); i++ ){

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
		glm::vec2 uv = temp_uvs[ uvIndex-1 ];
		glm::vec3 normal = temp_normals[ normalIndex-1 ];

		// Put the attributes in buffers
		out_vertices.push_back(vertex.x);
		out_vertices.push_back(vertex.y);
		out_vertices.push_back(vertex.z);

		out_uvs     .push_back(uv.x);
		out_uvs     .push_back(uv.y);

		out_normals .push_back(normal.x);
		out_normals .push_back(normal.y);
		out_normals .push_back(normal.z);
		out_normals .push_back(0.0);

		out_colors.push_back(1);
		out_colors.push_back(1);
		out_colors.push_back(1);

	}
	fclose(file);
	return true;
}

//Error handling
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods) {
    if (action==GLFW_PRESS) {
        if (key==GLFW_KEY_LEFT) speed_x=-PI/2;
        if (key==GLFW_KEY_RIGHT) speed_x=PI/2;
        if (key==GLFW_KEY_UP) speed_y=PI/2;
        if (key==GLFW_KEY_DOWN) speed_y=-PI/2;

        if (key==GLFW_KEY_W) speed_z=fly_speed;
        if (key==GLFW_KEY_S) speed_z=-fly_speed;
    }
    if (action==GLFW_RELEASE) {
        if (key==GLFW_KEY_LEFT) speed_x=0;
        if (key==GLFW_KEY_RIGHT) speed_x=0;
        if (key==GLFW_KEY_UP) speed_y=0;
        if (key==GLFW_KEY_DOWN) speed_y=0;

        if (key==GLFW_KEY_W) speed_z=0;
        if (key==GLFW_KEY_S) speed_z=0;
    }
}

float randomFloat(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
    glViewport(0,0,width,height);
}

GLuint readTexture(const char* filename) {
  GLuint tex;
  glActiveTexture(GL_TEXTURE0);

  //Get into memory
  std::vector<unsigned char> image;
  unsigned width, height;
  //Load image
  unsigned error = lodepng::decode(image, width, height, filename);

  //Import to graphics card memory
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*) image.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  return tex;
}

//Init procedure
void initOpenGLProgram(GLFWwindow* window) {
	glClearColor(0.356, 0.576, 0.901,1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window,windowResizeCallback);
	glfwSetKeyCallback(window,keyCallback);

	srand (time(NULL));

    bool res = loadOBJ("spher.obj", m_vertices, m_uvs, m_normals, m_colors);

    for(int i = 1; i < m_vertices.size(); i+=3){
        vec3 r = vec3(m_vertices[i-1],m_vertices[i],m_vertices[i+1]);
        if(m_vertices[i] > branch_thresh && find(m_growth_vertices.begin(),m_growth_vertices.end(),r) == m_growth_vertices.end()){
            m_growth_vertices.push_back(r);
        }
    }


    for(int i = 0; i < 2000; i++){
        random_angles.push_back(randomFloat(-max_angle,0));
        random_branch_numbers.push_back(rand()%(m_growth_vertices.size()/4)+1);
    }

    res = loadOBJ("ground.obj", g_vertices, g_uvs, g_normals, g_colors);
    res = loadOBJ("leaves.obj", l_vertices, l_uvs, l_normals, l_colors);

    bark=readTexture("bark.png");
    sky=readTexture("sky.png");
    ground = readTexture("grass_tex.png");
    leaves = readTexture("leaves.png");

	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
}

//Resource dealocation
void freeOpenGLProgram(GLFWwindow* window) {
    glDeleteTextures(1,&bark);
    glDeleteTextures(1,&sky);
    delete sp;
}

vec3 to_cross = vec3(0,1,0);
vec3 to_scale = vec3(0.518f,0.518f,0.518f);
vec3 leaves_scale = vec3(5.0f,5.0f,5.0f);
vec3 branch_point;
vec3 dir;
vec3 rot;
int rot_index = 0;
int branch_index = 0;
void drawTree(int level, mat4 M){

    if(level < tree_levels){
        int local_b = branch_index;

        for(int i = 0; i < random_branch_numbers[local_b]; i++){

            glDisableVertexAttribArray(sp->a("texCoord0"));

            glUniform1i(sp->u("textureMap0"),0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,bark);

            glUniform1i(sp->u("textureMap1"),1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,sky);

            glEnableVertexAttribArray(sp->a("vertex"));
            glVertexAttribPointer(sp->a("vertex"),3,GL_FLOAT,false,0,&m_vertices[0]);

            glEnableVertexAttribArray(sp->a("normal"));
            glVertexAttribPointer(sp->a("normal"),4,GL_FLOAT,false,0,&m_normals[0]);

            glEnableVertexAttribArray(sp->a("texCoord0"));
            glVertexAttribPointer(sp->a("texCoord0"),2,GL_FLOAT,false,0,&m_uvs[0]);

            branch_point = m_growth_vertices[i];
            dir = branch_point - vec3(0,0,0);
            rot = cross(dir,to_cross);

            mat4 P = translate(M,branch_point);
            P = rotate(P,random_angles[rot_index],rot);
            P = scale(P,to_scale);

            float scaler = my_time/(float)((level*3)+2);
            if(scaler < 1.0f)P = scale(P,vec3(scaler, scaler, scaler));

            int local_rot_index = rot_index;
            rot_index++;
            branch_index++;

            glUniformMatrix4fv(sp->u("M"),1,false,value_ptr(M));
            glDrawArrays(GL_TRIANGLES,0,m_vertices.size()/3);

            drawTree(level+1,P);

        }
    }else if(level==tree_levels){
        glDisableVertexAttribArray(sp->a("texCoord0"));

        glUniform1i(sp->u("textureMap0"),0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,leaves);

        glUniform1i(sp->u("textureMap1"),1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,sky);

        glEnableVertexAttribArray(sp->a("vertex"));
        glVertexAttribPointer(sp->a("vertex"),3,GL_FLOAT,false,0,&l_vertices[0]);

        glEnableVertexAttribArray(sp->a("normal"));
        glVertexAttribPointer(sp->a("normal"),4,GL_FLOAT,false,0,&l_normals[0]);

        glEnableVertexAttribArray(sp->a("texCoord0"));
        glVertexAttribPointer(sp->a("texCoord0"),2,GL_FLOAT,false,0,&l_uvs[0]);

        branch_point = m_growth_vertices[0];

        mat4 P = translate(M,branch_point);
        P = scale(P,leaves_scale);

        float scaler = my_time/(float)((level*1.2f)+2);
        if(scaler < 1.0f)P = scale(P,vec3(scaler, scaler, scaler));

        glUniformMatrix4fv(sp->u("M"),1,false,value_ptr(P));
        glDrawArrays(GL_TRIANGLES,0,l_vertices.size()/3);

    }

}

float scaler = 0.01f;
vec3 position = vec3(0,0,0);

void drawScene(GLFWwindow* window,float angle_x,float angle_y,float dis_z) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 V=lookAt(
         vec3(0, 0, -35),
         vec3(0,0,0),
         vec3(0.0f,1.0f,0.0f)); //Calculate view matrix

    mat4 P=perspective(50.0f*PI/180.0f, aspectRatio, 0.01f, 80.0f); //Calculate projection matrix

    vec3 my_front = vec3(0,0,0);
    my_front.x = float(sin(angle_x));//move forward and initialise speed
    my_front.z = -float(cos(angle_x));
    my_front.y = -float(sin(angle_y));


    P = translate(P,vec3(0,0,0));

	P=rotate(P,angle_y,vec3(1.0f,0.0f,0.0f)); //Calculate model matrix
	P=rotate(P,angle_x,vec3(0.0f,1.0f,0.0f)); //Calculate model matrix

    P = translate(P,position);

    P = translate(P,-dis_z*normalize(my_front));
    position+=(-dis_z*normalize(my_front));


    sp->use();//Activate the shader

    glUniformMatrix4fv(sp->u("P"),1,false,value_ptr(P));
    glUniformMatrix4fv(sp->u("V"),1,false,value_ptr(V));
    glUniform4f(sp->u("lp"),50,50,0,1); //Light source coordinates

    glUniform1i(sp->u("textureMap0"),0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,bark);

    glUniform1i(sp->u("textureMap1"),1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,sky);


    //Object drawing
    //TREE
    glEnableVertexAttribArray(sp->a("vertex"));  
    glVertexAttribPointer(sp->a("vertex"),3,GL_FLOAT,false,0,&m_vertices[0]); 

    glEnableVertexAttribArray(sp->a("normal"));
    glVertexAttribPointer(sp->a("normal"),4,GL_FLOAT,false,0,&m_normals[0]);

    glEnableVertexAttribArray(sp->a("texCoord0"));  
    glVertexAttribPointer(sp->a("texCoord0"),2,GL_FLOAT,false,0,&m_uvs[0]); 

    //The main stump
    mat4 M=mat4(1.0f);

    rot_index = 0;
    branch_index = 0;

    if(scaler<1.0f)scaler += (my_time/450.0f);
    M = scale(M,vec3(scaler,scaler,scaler));

    glUniformMatrix4fv(sp->u("M"),1,false,value_ptr(M));
    glDrawArrays(GL_TRIANGLES,0,m_vertices.size()/3);

    drawTree(1,M);


    //GROUND
    glDisableVertexAttribArray(sp->a("texCoord0"));

    glUniform1i(sp->u("textureMap0"),0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,ground);

    glUniform1i(sp->u("textureMap1"),1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,sky);


    M=mat4(1.0f);

    M = scale(M,vec3(50.0,50.0,50.0));
    M = translate(M,vec3(0.0,-0.02f,0));
    glUniformMatrix4fv(sp->u("M"),1,false,value_ptr(M));

    glEnableVertexAttribArray(sp->a("vertex"));  
    glVertexAttribPointer(sp->a("vertex"),3,GL_FLOAT,false,0,&g_vertices[0]); 

    glEnableVertexAttribArray(sp->a("normal"));
    glVertexAttribPointer(sp->a("normal"),4,GL_FLOAT,false,0,&g_normals[0]);

    glEnableVertexAttribArray(sp->a("texCoord0"));  
    glVertexAttribPointer(sp->a("texCoord0"),2,GL_FLOAT,false,0,&g_uvs[0]);

    glDrawArrays(GL_TRIANGLES,0,g_vertices.size()/3);


    glDisableVertexAttribArray(sp->a("vertex"));
    glDisableVertexAttribArray(sp->a("normal"));
    glDisableVertexAttribArray(sp->a("texCoord0"));


    glfwSwapBuffers(window);

}

int main(void)
{
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) { 
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL); 

	if (!window)
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	//Main loop
	float angle_x=0; 
	float angle_y=0; 
	float dis_z;
	glfwSetTime(0); 
	while (!glfwWindowShouldClose(window))
	{
	    my_time += glfwGetTime();

        angle_x+=speed_x*glfwGetTime(); 
        angle_y+=speed_y*glfwGetTime(); 
        dis_z=speed_z*glfwGetTime();

        glfwSetTime(0); 
		drawScene(window,angle_x,angle_y,dis_z); 
		glfwPollEvents(); 
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); 
	glfwTerminate(); 
	exit(EXIT_SUCCESS);
}

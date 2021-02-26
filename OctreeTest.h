#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Geometries.h"
#include "arcball_camera.h"
#include "Octree.h"
#include "Box.h"


static ArcballCamera arcballCamera({ 4,3,3 }, { 0,0,0 }, { 0,1,0 });
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorCallback(GLFWwindow* window, double x, double y);
void scrollCallback(GLFWwindow* window, double x, double y);

class OctreeTest
{
public:
	void Setup(GLFWwindow* window, int w_width, int w_height)
	{

		Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
		Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");
		Shader boxShader("QuadTree/shaders/box.vert", "QuadTree/shaders/box.frag");

		program = debugshader.createProgram();
		gridProgram = gridShader.createProgram();
		boxProgram = boxShader.createProgram();

		arcballCamera.setWH(w_width, w_height);

		Projection = glm::perspective(glm::radians(45.0f), (float)w_width / (float)w_height, 0.01f, 100.0f);

		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(0, 1, 0));

		View = arcballCamera.transform();

		mvp = Projection * View * Model;
		mvpId = glGetUniformLocation(program, "MVP");
		gridProjId = glGetUniformLocation(gridProgram, "proj");
		gridViewId = glGetUniformLocation(gridProgram, "view");

		boxModelId = glGetUniformLocation(boxProgram, "model");
		boxViewId = glGetUniformLocation(boxProgram, "view");
		boxProjId = glGetUniformLocation(boxProgram, "projection");


		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		this->window = window;

		box.init();
		cubePositions = {
		   { 1.0f, 3.0f, -5.0f },
		   { -7.25f, 2.1f, 1.5f },
		   { -15.0f, 2.55f, 9.0f },
		   { 4.0f, -3.5f, 5.0f },
		   { 2.8f, 1.9f, -6.2f },
		   { 3.5f, 6.3f, -1.0f },
		   { -3.4f, 10.9f, -5.5f },
		   { 0.0f, 11.0f, 0.0f },
		   { 0.0f, 5.0f, 0.0f }
		};

		octree = new Octree::Node(BoundingRegion(glm::vec3(-20), glm::vec3(20)));

		for (int i = 0; i < 9; i++)
		{
			glm::vec3 someSize = glm::vec3(0.1f);
			octree->addToPending(BoundingRegion(cubePositions[i] - someSize, cubePositions[i] + someSize));
		}
		octree->Update(box);
		octree->Update(box);
		
		glfwSetScrollCallback(window, scrollCallback);
		glfwSetCursorPosCallback(window, cursorCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
	}

	void Draw()
	{
		octree->Destroy();
		box.positions.clear();
		box.sizes.clear();

		octree = new Octree::Node(BoundingRegion(glm::vec3(-20), glm::vec3(20)));
		
		for (int i = 0; i < 9; i++)
		{
			glm::vec3 someSize = glm::vec3(0.1f);
			octree->addToPending(BoundingRegion(cubePositions[i] - someSize, cubePositions[i] + someSize));
			cubePositions[i] += 0.01f;
		}
		octree->Update(box);
		octree->Update(box);


		View = arcballCamera.transform();
		mvp = Projection * View * Model;

		glUseProgram(program);
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);
		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
		
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

	
		glm::mat4 boxModelMat = glm::mat4(1.0f);

		glUseProgram(boxProgram);
		glUniformMatrix4fv(boxModelId, 1, GL_FALSE, &boxModelMat[0][0]);
		glUniformMatrix4fv(boxViewId, 1, GL_FALSE, &View[0][0]);
		glUniformMatrix4fv(boxProjId, 1, GL_FALSE, &Projection[0][0]);

		box.render();

		glUseProgram(gridProgram);
		glUniformMatrix4fv(gridProjId, 1, GL_FALSE, &Projection[0][0]);
		glUniformMatrix4fv(gridViewId, 1, GL_FALSE, &View[0][0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		if (glfwGetKey(this->window, GLFW_KEY_R)) 
		{
			Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
			Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");
			Shader boxShader("QuadTree/shaders/box.vert", "QuadTree/shaders/box.frag");

			program = debugshader.createProgram();
			gridProgram = gridShader.createProgram();
			boxProgram = boxShader.createProgram();
		}
	}

private:
	int w_width;
	int w_height;

	glm::mat4 mvp;
	GLuint mvpId;
	GLuint gridProjId;
	GLuint gridViewId;

	GLuint boxProjId;
	GLuint boxViewId;
	GLuint boxModelId;

	GLuint program;
	GLuint gridProgram;
	GLuint boxProgram;

	GLuint vertexbuffer;
	GLuint colorbuffer;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;

	Box box;
	// pointer to root node in octree
	Octree::Node* octree;

	std::vector<glm::vec3> cubePositions;

	GLFWwindow* window;
};


void scrollCallback(GLFWwindow* window, double x, double y)
{
	//arcball.scrollCallback(window, x, y);
	arcballCamera.scrollCallback(window, x, y);
}

void cursorCallback(GLFWwindow* window, double x, double y)
{
	//arcball.cursorCallback(window, x, y);
	arcballCamera.cursorCallback(window, x, y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//arcball.mouseButtonCallback(window, button, action, mods);
	arcballCamera.mouseButtonCallback(window, button, action, mods);
}
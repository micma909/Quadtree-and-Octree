#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Geometries.h"
#include "arcball_camera.h"

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

		program = debugshader.createProgram();
		gridProgram = gridShader.createProgram();

		arcballCamera.setWH(w_width, w_height);

		Projection = glm::perspective(glm::radians(45.0f), (float)w_width / (float)w_height, 0.01f, 100.0f);

		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(0, 1, 0));

		View = arcballCamera.transform();

		mvp = Projection * View * Model;
		mvpId = glGetUniformLocation(program, "MVP");
		gridProjId = glGetUniformLocation(gridProgram, "proj");
		gridViewId = glGetUniformLocation(gridProgram, "view");

		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);


		this->window = window;

		glfwSetScrollCallback(window, scrollCallback);
		glfwSetCursorPosCallback(window, cursorCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
	}

	void Draw()
	{
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

		glUseProgram(gridProgram);
		glUniformMatrix4fv(gridProjId, 1, GL_FALSE, &Projection[0][0]);
		glUniformMatrix4fv(gridViewId, 1, GL_FALSE, &View[0][0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (glfwGetKey(this->window, GLFW_KEY_R)) 
		{
			Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
			Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");

			program = debugshader.createProgram();
			gridProgram = gridShader.createProgram();
		}
	}

private:
	int w_width;
	int w_height;

	glm::mat4 mvp;
	GLuint mvpId;
	GLuint gridProjId;
	GLuint gridViewId;
	GLuint program;
	GLuint gridProgram;

	GLuint vertexbuffer;
	GLuint colorbuffer;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;

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
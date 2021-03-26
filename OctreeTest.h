#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "arcball_camera.h"
#include "Octree.h"
#include "Box.h"
#include "Instance.h"



static ArcballCamera arcballCamera({ 40,40,50 }, { 0,20,0 }, { 0,1,0 });
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorCallback(GLFWwindow* window, double x, double y);
void scrollCallback(GLFWwindow* window, double x, double y);

class OctreeTest
{
public:
	float RandomFloat(float min, float max)
	{
		assert(max > min);
		float random = ((float)rand()) / (float)RAND_MAX;
		float range = max - min;
		return (random * range) + min;
	}

	void Setup(GLFWwindow* window, int w_width, int w_height)
	{

		Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
		Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");
		Shader boxShader("QuadTree/shaders/box.vert", "QuadTree/shaders/box.frag");

		program = debugshader.createProgram();
		gridProgram = gridShader.createProgram();
		boxProgram = boxShader.createProgram();

		arcballCamera.setWH(w_width, w_height);

		Projection = glm::perspective(glm::radians(45.0f), (float)w_width / (float)w_height, 0.01f, 1000.0f);

		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(0, 0, 0));

		View = arcballCamera.transform();

		mvp = Projection * View * Model;
		mvpId = glGetUniformLocation(program, "MVP");
		gridProjId = glGetUniformLocation(gridProgram, "proj");
		gridViewId = glGetUniformLocation(gridProgram, "view");

		boxModelId = glGetUniformLocation(boxProgram, "model");
		boxViewId = glGetUniformLocation(boxProgram, "view");
		boxProjId = glGetUniformLocation(boxProgram, "projection");

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		this->window = window;

		box.init();
		int nrPoints = 1000;
		for (int i = 0; i < nrPoints; i++)
		{
			positions.push_back(glm::vec3(RandomFloat(-1, 1), RandomFloat(5, 25), RandomFloat(-10, 10)));
		
			//velocities.push_back(glm::vec3(RandomFloat(-1, 1), RandomFloat(-1, 1), RandomFloat(-1, 1)));
			velocities.push_back(glm::vec3(-1, 0.1f, 0));
			colors.push_back(glm::vec3(1));
		}

		glGenBuffers(1, &pointBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

		octree = new Octree::Node(BoundingRegion(20, glm::vec3(0, 20, 0)));
		octree->region.debugColor = glm::vec4(1, 0, 0, 1);

		for (int i = 0; i < positions.size(); i++)
		{
			glm::vec3 someSize = glm::vec3(0.05f);
			instances.push_back(Instance(positions[i], glm::vec3(1.0f)));
			boundingRegions.push_back(BoundingRegion(-someSize, +someSize));
		}

		for (int i = 0; i < positions.size(); i++)
		{
			boundingRegions[i].instance = &instances[i];
			boundingRegions[i].instance->id = i;
			octree->addToPending(boundingRegions[i]);
		}
		octree->Update();

		glfwSetScrollCallback(window, scrollCallback);
		glfwSetCursorPosCallback(window, cursorCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
	}

	void Draw()
	{
		int timeState = glfwGetKey(window, GLFW_KEY_SPACE);
		if (timeState == GLFW_RELEASE && oldTimeState == GLFW_PRESS)
		{
			pauseTime = !pauseTime;
		}
		oldTimeState = timeState;

		box.positions.clear();
		box.sizes.clear();
		box.colors.clear();

		BoundingRegion searchRegion(glm::vec3(-19, -1, -19), glm::vec3(19, 41, 19));
		box.positions.push_back(searchRegion.calculateCenter());
		box.sizes.push_back(searchRegion.calculateDimensions());
		box.colors.push_back(glm::vec4(1));

		if(pauseTime)
			for (int i = 0; i < positions.size(); i++)
			{
				glm:vec3 nextStep = 0.1f * velocities[i];
				glm::vec3 nextPos = instances[i].position + nextStep;
				
				BoundingRegion tmp = boundingRegions[i];
				tmp.transform();
				if (octree->region.containsRegion(tmp) && boundingRegions[i].outOfBounds)
				{
					octree->addToPending(boundingRegions[i]);
					boundingRegions[i].outOfBounds = false;
				} 
				
				if(!searchRegion.containsRegion(tmp))
				{
					searchRegion.reflectOnBounds(nextPos, velocities[i]);
					nextStep = 0.1f * velocities[i];
					boundingRegions[i].outOfBounds = true;
				}

				instances[i].position += nextStep;
				
				
				positions[i] = instances[i].position; // this is a bit stupid atm but ok
				
				
				if(glm::length(velocities[i]) > 0.f)
					states::activate(&boundingRegions[i].instance->state, INSTANCE_MOVED);
				else
					states::deactivate(&boundingRegions[i].instance->state, INSTANCE_MOVED);
			}


		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

		octree->Update();
		octree->RemoveStaleBranches();
		octree->Draw(box);

		View = arcballCamera.transform();
		mvp = Projection * View * Model;

		glUseProgram(program);
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);
		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glPointSize(3);
		glDrawArrays(GL_POINTS, 0, positions.size());

		std::vector<glm::vec3> foundPoints;
		octree->Search(searchRegion, foundPoints);

 		if (foundPoints.size())
		{
			glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * foundPoints.size(), &foundPoints[0].x, GL_DYNAMIC_DRAW);

			colors.clear();
			for (int i = 0; i < foundPoints.size(); i++)
			{
				colors.push_back(glm::vec3(1, 0.6, 0));
			}

			glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glPointSize(10);
			glDrawArrays(GL_POINTS, 0, positions.size());
			colors.clear();

			for (int i = 0; i < positions.size(); i++)
			{
				colors.push_back(glm::vec3(1,1,1));
			}

			glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);
		}
		
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

	int oldTimeState;
	bool pauseTime = false;

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

	GLuint colorbuffer;
	GLuint pointBuffer;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;

	Box box;
	// pointer to root node in octree
	Octree::Node* octree;

	std::vector<BoundingRegion> boundingRegions;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> velocities;
	std::vector<glm::vec3> colors;

	std::vector<Instance> instances;

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
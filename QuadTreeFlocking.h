#pragma once
#include <iostream>
#include "QuadTree.h"
#include "Util.h"

#include "Boid.h"

#define DATA_ORIENTED

class QTFlocking
{
public:
	QTFlocking(GLFWwindow* window, int width, int height, int pointCount) : w_width(width)
		, w_height(height)
		, nrOfBoids(pointCount)
		, window(window)
	{}

	void data_align()
	{
		float perceptionRadius = 50.0f;
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < perceptionRadius)
					{
						steering += velocities[j];
						count++;
					}
				}
			}
	
			if (count)
			{
				steering /= count;
				steering = glm::normalize(steering) * ppMaxSpeed[i];
				steering -= velocities[i];
				float length = glm::length(steering);
				if (length > ppMaxForce[i])
				{
					steering = glm::normalize(steering) * ppMaxForce[i];
				}
			}
			accelerations[i] += steering;
			steering *= 0.f;
			count = 0;
		}
	}

	void data_cohesion()
	{
		float perceptionRadius = 100.0f;
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < perceptionRadius)
					{
						steering += positions[j];
						count++;
					}
				}
			}
	
			if (count > 0)
			{
				steering /= count;
				steering -= positions[i];
				
				if(glm::length(steering) > 0)
					steering = glm::normalize(steering) * ppMaxSpeed[i];
				steering -= velocities[i];
				
				if (glm::length(steering) > ppMaxForce[i])
					steering = glm::normalize(steering) * ppMaxForce[i];
			}
			accelerations[i] += steering;
			steering *= 0.f;
			count = 0;
		}
	}

	void data_separation()
	{
		float perceptionRadius = 100.0f;
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < perceptionRadius)
					{
						glm::vec2 diff = positions[i] - positions[j];
						diff /= d;

						steering += diff;
						count++;
					}
				}
			}

			if (count > 0)
			{
				steering /= count;
				if (glm::length(steering) > 0)
					steering = glm::normalize(steering) * ppMaxSpeed[i];
				steering -= velocities[i];
				if (glm::length(steering) > ppMaxForce[i])
					steering = glm::normalize(steering) * ppMaxForce[i];
			}
			accelerations[i] += steering;
			steering *= 0.f;
			count = 0;
		}
	}


	void Setup()
	{
#ifndef DATA_ORIENTED
		for(int i = 0; i < nrOfBoids; i++)
			boids.push_back({ w_width, w_height });
#else
		// data oriented approach:
		for (int i = 0; i < nrOfBoids; i++)
		{
			positions.push_back({ RandomFloat(0, w_width), RandomFloat(0, w_height) });
			velocities.push_back({ RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f) });
			accelerations.push_back({ 0.f, 0.f });
			ppRadius.push_back(10.0f);
			ppMaxForce.push_back(0.2f);
			ppMaxSpeed.push_back(4.2f);
		}
#endif
	}

	void Run()
	{
#ifndef DATA_ORIENTED
		for (int i = 0; i < boids.size(); i++)
		{	
			borderCheck(&boids[i].position, w_width, w_height, true);
			boids[i].flock(&boids);
			boids[i].update();
			drawBoid(&boids[i]);
		}
#else
		data_align();
		data_cohesion();
		data_separation();

		for (int i = 0; i < positions.size(); i++)
		{
			borderCheck(&positions[i], w_width, w_height, true);
			
			// update
			positions[i] += velocities[i];
			velocities[i] += accelerations[i];
			if (glm::length(velocities[i]) > ppMaxSpeed[i])
				velocities[i] = glm::normalize(velocities[i]) * ppMaxSpeed[i];

			accelerations[i] *= 0.0f;
			drawPoint(positions[i], ppRadius[i]);
		}
#endif
	}

private:
	int w_width;
	int w_height;
	int nrOfBoids;

	GLFWwindow* window;
	std::vector<Boid> boids;

	std::vector<glm::vec2> positions;
	std::vector<glm::vec2> velocities;
	std::vector<glm::vec2> accelerations;
	std::vector<float> ppRadius;
	std::vector<float> ppMaxForce;
	std::vector<float> ppMaxSpeed;

};
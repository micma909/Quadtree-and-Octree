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

	void data_flocking()
	{
		glm::vec2 alignment(0.f, 0.f);
		glm::vec2 cohesion(0.f, 0.f);
		glm::vec2 separation(0.f, 0.f);
		int alignmentCount = 0;
		int cohesionCount = 0;
		int separationCount = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < this->alignmentRadius)
					{
						alignment += velocities[j];
						alignmentCount++;
					}

					if (d < this->cohesionRadius)
					{
						cohesion += positions[j];
						cohesionCount++;
					}

					if (d < this->separationRadius)
					{
						glm::vec2 diff = positions[i] - positions[j];
						diff /= d;

						separation += diff;
						separationCount++;
					}
				}
			}

			if (alignmentCount)
			{
				alignment /= alignmentCount;
				alignment = glm::normalize(alignment) * alignmentMaxSpeed;
				alignment -= velocities[i];
				if (glm::length(alignment) > alignmentMaxForce)
				{
					alignment = glm::normalize(alignment) * alignmentMaxForce;
				}
			}

			if (cohesionCount > 0)
			{
				cohesion /= cohesionCount;
				cohesion -= positions[i];

				if (glm::length(cohesion) > 0)
					cohesion = glm::normalize(cohesion) * cohesionMaxSpeed;
				cohesion -= velocities[i];

				if (glm::length(cohesion) > cohesionMaxForce)
					cohesion = glm::normalize(cohesion) * cohesionMaxForce;
			}

			if (separationCount > 0)
			{
				separation /= separationCount;
				if (glm::length(separation) > 0)
					separation = glm::normalize(separation) * separationMaxSpeed;
				separation -= velocities[i];
				if (glm::length(separation) > separationMaxForce)
					separation = glm::normalize(separation) * separationMaxForce;
			}

			accelerations[i] += alignment * alignmentCoeff;
			accelerations[i] += cohesion * cohesionCoeff;
			accelerations[i] += separation * separationCoeff;
			// reset
			alignment *= 0.f;
			cohesion *= 0.f;
			separation *= 0.f;
			alignmentCount = 0;
			cohesionCount = 0;
			separationCount = 0;
		}
	}


	void data_align()
	{
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < this->alignmentRadius)
					{
						steering += velocities[j];
						count++;
					}
				}
			}
	
			if (count)
			{
				steering /= count;
				steering = glm::normalize(steering) * alignmentMaxSpeed;
				steering -= velocities[i];
				if (glm::length(steering) > alignmentMaxForce)
				{
					steering = glm::normalize(steering) * alignmentMaxForce;
				}
			}
			accelerations[i] += steering * alignmentCoeff;
			steering *= 0.f;
			count = 0;
		}
	}

	void data_cohesion()
	{
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < this->cohesionRadius)
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
					steering = glm::normalize(steering) * cohesionMaxSpeed;
				steering -= velocities[i];
				
				if (glm::length(steering) > cohesionMaxForce)
					steering = glm::normalize(steering) * cohesionMaxForce;
			}
			accelerations[i] += steering * cohesionCoeff;
			steering *= 0.f;
			count = 0;
		}
	}

	void data_separation()
	{
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			
			for (int j = 0; j < positions.size(); j++)
			{
				if (i != j)
				{
					float d = distance(&positions[i], &positions[j]);
					if (d < this->separationRadius)
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
					steering = glm::normalize(steering) * separationMaxSpeed;
				steering -= velocities[i];
				if (glm::length(steering) > separationMaxForce)
					steering = glm::normalize(steering) * separationMaxForce;
			}
			accelerations[i] += steering * separationCoeff;
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
			ppRadius.push_back(5.0f);
			ppColor.push_back( { 1, 1, 1, RandomFloat(0.2f, 1.0f)});
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
		//if (alignmentOn) data_align();
		//if (cohesionOn) data_cohesion();
		//if (separationOn) data_separation();

		data_flocking();

		for (int i = 0; i < positions.size(); i++)
		{
			borderCheck(&positions[i], w_width, w_height, true);
			
			// update
			positions[i] += velocities[i];
			velocities[i] += accelerations[i];
			//if (glm::length(velocities[i]) > ppMaxSpeed[i])
			//	velocities[i] = glm::normalize(velocities[i]) * ppMaxSpeed[i];

			accelerations[i] *= 0.0f;
			drawPoint(positions[i], ppRadius[i], ppColor[i]);
		}
#endif
	}

	void ImGuiMenu()
	{
		static float aR = 50.0f;
		static float cR = 100.0f;
		static float sR = 100.0f;

		static float aC = 1.0f;
		static float cC = 1.0f;
		static float sC = 1.0f;

		static float aMaxSpeed = 4.0f;
		static float aMaxForce = 0.2f;

		static float cMaxSpeed = 4.0f;
		static float cMaxForce = 0.2f;

		static float sMaxSpeed = 4.0f;
		static float sMaxForce = 0.2f;

		static int counter = 0;

		ImGui::Begin("Flocking control");

		//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		//ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::Checkbox("Alignment", &alignmentOn);
		ImGui::SliderFloat("radius_A", &aR, 0.0f, w_width); 
		this->alignmentRadius = aR;
		ImGui::SliderFloat("strength_A", &aC, 0.0f, 10.0f);
		this->alignmentCoeff = aC;
		ImGui::SliderFloat("max speed_A", &aMaxSpeed, 0.0f, 10.0f);
		this->alignmentMaxSpeed = aMaxSpeed;
		ImGui::SliderFloat("max force_A", &aMaxForce, 0.0f, 10.0f);
		this->alignmentMaxForce = aMaxForce;

		ImGui::Checkbox("Cohesion", &cohesionOn);
		ImGui::SliderFloat("radius_C", &cR, 0.0f, w_width);
		this->cohesionRadius = cR;
		ImGui::SliderFloat("strength_C", &cC, 0.0f, 10.0f);
		this->cohesionCoeff = cC;
		ImGui::SliderFloat("max speed_C", &cMaxSpeed, 0.0f, 10.0f);
		this->cohesionMaxSpeed = cMaxSpeed;
		ImGui::SliderFloat("max force_C", &cMaxForce, 0.0f, 10.0f);
		this->cohesionMaxForce = aMaxForce;

		ImGui::Checkbox("Separation", &separationOn);
		ImGui::SliderFloat("radius_S", &sR, 0.0f, w_width);
		this->separationRadius = sR;
		ImGui::SliderFloat("strength_S", &sC, 0.0f, 10.0f);
		this->separationCoeff = cC;
		ImGui::SliderFloat("max speed_S", &sMaxSpeed, 0.0f, 10.0f);
		this->separationMaxSpeed = sMaxSpeed;
		ImGui::SliderFloat("max force_S", &sMaxForce, 0.0f, 10.0f);
		this->separationMaxForce = sMaxForce;

		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
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
	std::vector<glm::vec4> ppColor;
	std::vector<float> ppRadius;

	float alignmentRadius;
	float cohesionRadius;
	float separationRadius;

	float alignmentCoeff;
	float cohesionCoeff;
	float separationCoeff;

	// max values
	float alignmentMaxForce;
	float alignmentMaxSpeed;

	float cohesionMaxForce;
	float cohesionMaxSpeed;	
	
	float separationMaxForce;
	float separationMaxSpeed;

	bool alignmentOn = true;
	bool cohesionOn = true;
	bool separationOn = true;

};
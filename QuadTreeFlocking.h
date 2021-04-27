#pragma once
#include <iostream>
#include "QuadTree.h"
#include "Util.h"

#include "Boid.h"

#define DATA_ORIENTED
static int frameCount = 0;

class QTFlocking
{
public:
	QTFlocking(GLFWwindow* window, int width, int height, int pointCount) : w_width(width)
		, w_height(height)
		, nrBoids(pointCount)
		, window(window)
	{}

	void data_flocking_naive()
	{
		glm::vec2 alignment(0.f, 0.f);
		glm::vec2 cohesion(0.f, 0.f);
		glm::vec2 separation(0.f, 0.f);

		int alignmentCount = 0;
		int cohesionCount = 0;
		int separationCount = 0;
		for (int i = 0; i < points.size(); i++)
		{
			for (int j = 0; j < points.size(); j++)
			{
				if (i != j)
				{
					float d = distance(points[i].pos, points[j].pos);
					if (d < this->alignmentRadius && alignmentOn)
					{
						alignment += velocities[j];
						alignmentCount++;
					}

					if (d < this->cohesionRadius && cohesionOn)
					{
						cohesion += points[j].pos;
						cohesionCount++;
					}

					if (d < this->separationRadius && separationOn)
					{
						glm::vec2 diff = points[i].pos - points[j].pos;
						diff /= d;

						separation += diff;
						separationCount++;
					}
				}
			}

			if (alignmentCount > 0 && alignmentOn)
			{
				alignment /= alignmentCount;
				alignment = glm::normalize(alignment) * alignmentMaxSpeed;
				alignment -= velocities[i];
				if (glm::length(alignment) > alignmentMaxForce)
				{
					alignment = glm::normalize(alignment) * alignmentMaxForce;
				}
			}

			if (cohesionCount > 0 && cohesionOn)
			{
				cohesion /= cohesionCount;
				cohesion -= points[i].pos;

				if (glm::length(cohesion) > 0)
					cohesion = glm::normalize(cohesion) * cohesionMaxSpeed;
				cohesion -= velocities[i];

				if (glm::length(cohesion) > cohesionMaxForce)
					cohesion = glm::normalize(cohesion) * cohesionMaxForce;
			}

			if (separationCount > 0 && separationOn)
			{
				separation /= separationCount;
				if (glm::length(separation) > 0)
					separation = glm::normalize(separation) * separationMaxSpeed;
				separation -= velocities[i];
				if (glm::length(separation) > separationMaxForce)
					separation = glm::normalize(separation) * separationMaxForce;
			}

			accelerations[frameIndex][i] += alignment;
			accelerations[frameIndex][i] += cohesion;
			accelerations[frameIndex][i] += separation;
			accelerations[frameIndex][i] *= 1.5;
			// reset
			alignment *= 0.f;
			cohesion *= 0.f;
			separation *= 0.f;
			alignmentCount = 0;
			cohesionCount = 0;
			separationCount = 0;
		}
	}

	void data_flocking_quad()
	{
		glm::vec2 alignment(0.f, 0.f);
		glm::vec2 cohesion(0.f, 0.f);
		glm::vec2 separation(0.f, 0.f);
		glm::vec2 avoidance(0.f, 0.f);

		int alignmentCount = 0;
		int cohesionCount = 0;
		int separationCount = 0;

		std::vector<Point> nearBoids;
		for (int i = 0; i < points.size(); i++)
		{
			BoidData& thisBoidsData = getBoidData(points[i].data);
			bool hunterDetected = false;
			ppAvoidance[i] *= 0.f;

			if (thisBoidsData.behaviorType == Behavior::Hunter && !addHunters)
				continue;

			if (thisBoidsData.behaviorType == Behavior::Boid)
			{
				glm::vec2& p1 = points[i].pos;
				glm::vec2 offsetFov = p1;

				if(glm::length(velocities[i]) > 0)
					offsetFov = p1 + glm::normalize(velocities[i]) * queryRadius;
				Rectangle searchArea(offsetFov.x, offsetFov.y, queryRadius, queryRadius);
				
				// fade color
				ppColor[i].x += 0.01f;
				ppColor[i].y += 0.01f;
				ppColor[i].z += 0.01f;
				ppColor[i].w -= 0.01f;
				ppColor[i].w = std::max(0.2f, ppColor[i].w);
				ppRadius[i] -= 0.05f;
				ppRadius[i] = std::max(2.5f, ppRadius[i]);

				qt->query(searchArea, &nearBoids);

				for (int j = 0; j < nearBoids.size(); j++)
				{
					float d = distance(p1, nearBoids[j].pos);
					if (d < 0.001f)
						continue;

					BoidData& otherBoidData = getBoidData(nearBoids[j].data);
					glm::vec2& p2 = nearBoids[j].pos;
					if (otherBoidData.behaviorType == Behavior::Boid)
					{
						if (d < this->alignmentRadius && alignmentOn)
						{
							int idx = otherBoidData.arrayIndex;
							alignment += velocities[idx];
							alignmentCount++;
						}

						if (d < this->cohesionRadius && cohesionOn)
						{
							cohesion += nearBoids[j].pos;
							cohesionCount++;
						}

						if (d < this->separationRadius && separationOn)
						{
							glm::vec2 diff = points[i].pos - nearBoids[j].pos;
							diff /= std::pow(d, 2);

							separation += diff;
							separationCount++;
						}
					}
					else if(otherBoidData.behaviorType == Behavior::Hunter && addHunters)
					{
						glm::vec2 diff = points[i].pos - nearBoids[j].pos;
						diff /= d;

						ppColor[i] = glm::vec4(0, 1, 1, 1);
						ppAvoidance[i] = 4.f*diff;
						ppSeparationForce[i] = 2;
						ppRadius[i] = 5.0f;
						alignment *= 0;
						cohesion *= 0;
						hunterDetected = true;
					}
				}
			}
			else if (thisBoidsData.behaviorType == Behavior::Hunter)
			{
				float hunterRadius = 300.0f;
				glm::vec2& p1 = points[i].pos;
				Rectangle hunterSearchArea(p1.x, p1.y, hunterRadius, hunterRadius);
				qt->query(hunterSearchArea, &nearBoids);
			
				for (int j = 0; j < nearBoids.size(); j++)
				{
					float d = glm::distance(p1, nearBoids[j].pos);
			
					if (d < hunterRadius && cohesionOn && frameCount % 20 == 0)
					{
						cohesion += nearBoids[j].pos;
						cohesionCount++;
					}
				}
			}

			if (!hunterDetected)
			{
				ppSeparationForce[i] -= 0.5f;
				ppSeparationForce[i] = std::max(ppSeparationForce[i], separationMaxForce);
			}

			if (alignmentCount > 0 && alignmentOn)
			{
				alignment /= alignmentCount;
				limit(alignment, alignmentMaxSpeed);
				alignment -= velocities[i];
				limit(alignment, alignmentMaxForce);
			}
			
			if (cohesionCount > 0 && cohesionOn)
			{
				cohesion /= cohesionCount;
				cohesion -= points[i].pos;
				limit(cohesion, cohesionMaxSpeed);
				cohesion -= velocities[i];
				limit(cohesion, cohesionMaxForce);
			}

			if (separationCount > 0 && separationOn)
			{
				separation /= separationCount;
				limit(separation, separationMaxSpeed);
				separation -= velocities[i];
				limit(separation, ppSeparationForce[i]);
			}

			if ( thisBoidsData.behaviorType == Behavior::Hunter)
			{
				if (cohesionCount < 40.0f)
					cohesion = accelerations[1 - frameIndex][i] * 0.9f;
				else
					cohesion *= 4.0f;
			}
	
			accelerations[frameIndex][i] += alignment;
			accelerations[frameIndex][i] += cohesion;
			accelerations[frameIndex][i] += separation;
			accelerations[frameIndex][i] += ppAvoidance[i];

			ppAvoidance[i] -= 0.01f;
			ppAvoidance[i] = glm::max(ppAvoidance[i], 0.0f);
			
			if (thisBoidsData.behaviorType == Behavior::Hunter)
			{
				accelerations[frameIndex][i] *= 2.5f;
			}

			// reset
			alignment *= 0.f;
			cohesion *= 0.f;
			separation *= 0.f;
			avoidance *= 0.f;
			alignmentCount = 0;
			cohesionCount = 0;
			separationCount = 0;
			nearBoids.clear();
		}	
	}

	std::vector<glm::vec2> GetQTPoints()
	{
		std::vector<glm::vec2> allPoints;

		for (int i = 0; i < points.size(); i++)
		{
			allPoints.push_back(points[i].pos);
		}

		return allPoints;
	}

	void Setup()
	{
#ifndef DATA_ORIENTED
		for(int i = 0; i < nrOfBoids; i++)
			boids.push_back({ w_width, w_height });
#else
		points.clear();
		velocities.clear();
		ppRadius.clear();
		ppColor.clear();
		for(int i = 0; i < 2; i++)
		accelerations[i].clear();

		if(qt != nullptr)
			qt->clear();

		nrHunters = 4;
		this->treeLevelCapacity = 10;
		Rectangle boundary(w_width / 2, w_height / 2, w_width / 2, w_height / 2);
		qt = new QuadTree(boundary, this->treeLevelCapacity);

		// data oriented approach:
		int boidCount = 0;
		for (int i = 0; i < nrHunters; i++)
		{   
			BoidData boidData = { Behavior::Hunter , boidCount };
		 
			points.push_back({ RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f), boidData });
			velocities.push_back({ RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f) });
			for(int j = 0; j < 2; j++)
				accelerations[j].push_back({ 0.f, 0.f });
			ppRadius.push_back(10.f);
			ppColor.push_back({ 1, 0.6, 0, 1 });
			ppSeparationForce.push_back(0.f);
			ppAvoidance.push_back({ 0.f, 0.f });

			// build quadtree
			qt->insert(&points[i]);
			boidCount++;
		}

		for (int i = 0; i < nrBoids; i++)
		{
			BoidData boidData = { Behavior::Boid , boidCount };
			points.push_back({ RandomFloat(0, w_width), RandomFloat(0, w_height), boidData });
			velocities.push_back({ 0, 0});
			for (int j = 0; j < 2; j++)
				accelerations[j].push_back({ 0.f, 0.f });
			ppRadius.push_back(2.5f);
			ppColor.push_back( { 1, 1, 1, 0.3f});
			ppSeparationForce.push_back(separationMaxForce);
			ppAvoidance.push_back({ 0.f, 0.f });

			// build quadtree
			qt->insert(&points[i]);
			boidCount++;
		}

		// opengl stuff
		Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
		program = shader.createProgram();

		positions = GetQTPoints();

		glGenBuffers(1, &vboPositions);
		glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &vboColors);
		glBindBuffer(GL_ARRAY_BUFFER, vboColors);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * ppColor.size(), &ppColor[0].x, GL_DYNAMIC_DRAW);

		std::vector<glm::vec2> lines;
		qt->GetBoundsLineSegments(lines);

		mvp = glm::ortho(0.f, static_cast<float>(w_width+500), 0.f, static_cast<float>(w_height), 0.f, 100.f);
		mvpId = glGetUniformLocation(program, "MVP");

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#endif
	}

	void Draw()
	{
		glUseProgram(program);
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vboColors);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * ppColor.size(), &ppColor[0].x, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

		positions = GetQTPoints();
		if (positions.size())
		{
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glPointSize(3);
			glDrawArrays(GL_POINTS, 0, positions.size());

			positions.clear();
			lines.clear();
			qt->GetBoundsLineSegments(lines);
			if (lines.size() && quadTree)
			{
				glDisableVertexAttribArray(1);
				glVertexAttrib4f(1, 0.15f, 0.15f, 0.15f, 1.f);

				GLuint vboLines;
				glGenBuffers(1, &vboLines);
				glBindBuffer(GL_ARRAY_BUFFER, vboLines);
				glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * lines.size(), &lines[0].x, GL_DYNAMIC_DRAW);

				glBindBuffer(GL_ARRAY_BUFFER, vboLines);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glDrawArrays(GL_LINES, 0, lines.size());
				glDisableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
			}
		}
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
		if (quadTree)
		{
			data_flocking_quad();
		}
		else
		{
			data_flocking_naive();
		}
		
		qt->clear(treeLevelCapacity);
		for (int i = 0; i < points.size(); i++)
		{
			borderCheck(&points[i].pos, w_width, w_height, true);
			
			// update
			points[i].pos += velocities[i];
			velocities[i] += accelerations[frameIndex][i];
			// TODO: Make velocities doublebuffer!
			accelerations[frameIndex][i] *= 0.0f;

			BoidData& thisBoidsData = getBoidData(points[i].data);
			bool hunterDetected = false;
			ppAvoidance[i] *= 0.f;

			if (thisBoidsData.behaviorType == Behavior::Hunter && !addHunters)
				continue;

			drawPoint(points[i].pos, velocities[i], ppRadius[i], ppColor[i]);

			// rebuild quadtree
			bool success = qt->insert(&points[i]);
		}

		frameCount++;
		frameIndex = !frameIndex;
#endif
	}

	void ImGuiMenu()
	{
		static float qR = 65.0f;

		static float aR = 65.0f;
		static float cR = 100.0f;
		static float sR = 30.0f;

		static float aMaxSpeed = 40.f;
		static float aMaxForce = 1.2f;

		static float cMaxSpeed = 6.0f;
		static float cMaxForce = 1.1f;

		static float sMaxSpeed = 20.f;
		static float sMaxForce = 2.0f;

		static int counter = 0;
		static int treeCapacity = 10;

		static float maxSearchRadius = 150.f;

		ImVec4 col_text = ImColor(255, 255, 255);
		ImVec4 col_main = ImColor(30, 30, 30);
		ImVec4 col_oran = ImColor(255, 185, 0);
		ImVec4 col_doran = ImColor(255, 150, 0);
		ImVec4 col_area = ImColor(51, 56, 69);
		ImGuiStyle& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_FrameBg] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f); 
		style.Colors[ImGuiCol_Text] = ImVec4(0,0, 0, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(col_doran.x, col_doran.y, col_doran.z, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 0.90f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);

		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoBackground;
		//ImGui::Begin("Dear ImGui Demo", p_open, window_flags)
		ImGui::Begin("Flocking control", nullptr, window_flags);
		style.Colors[ImGuiCol_Text] = ImVec4(255, 255, 255, 1.00f);
		ImGui::SliderInt("QT Level Capacity", &treeCapacity, 1, 100);
		this->treeLevelCapacity = treeCapacity;
		
		ImGui::SliderFloat("Query Radius", &qR, 0.0f, maxSearchRadius);
		this->queryRadius = qR;

		ImGui::Checkbox("Alignment", &alignmentOn);
		ImGui::SliderFloat("radius_A", &aR, 0.0f, qR);
		this->alignmentRadius = aR;
		ImGui::SliderFloat("max speed_A", &aMaxSpeed, 0.0f, 40.0f);
		this->alignmentMaxSpeed = aMaxSpeed;
		ImGui::SliderFloat("max force_A", &aMaxForce, 0.0f, 20.0f);
		this->alignmentMaxForce = aMaxForce;



		ImGui::Checkbox("Cohesion", &cohesionOn);
		ImGui::SliderFloat("radius_C", &cR, 0.0f, maxSearchRadius);
		this->cohesionRadius = cR;
		ImGui::SliderFloat("max speed_C", &cMaxSpeed, 0.0f, 20.0f);
		this->cohesionMaxSpeed = cMaxSpeed;
		ImGui::SliderFloat("max force_C", &cMaxForce, 0.0f, 20.0f);
		this->cohesionMaxForce = aMaxForce;

		

		ImGui::Checkbox("Separation", &separationOn);
		ImGui::SliderFloat("radius_S", &sR, 0.0f, maxSearchRadius);
		this->separationRadius = sR;
		ImGui::SliderFloat("max speed_S", &sMaxSpeed, 0.0f, 20.0f);
		this->separationMaxSpeed = sMaxSpeed;
		ImGui::SliderFloat("max force_S", &sMaxForce, 0.0f, 20.0f);
		this->separationMaxForce = sMaxForce;

		
		ImGui::Checkbox("QuadTree", &quadTree);
		if (quadTree)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Draw QuadTree", &drawQuadTree);
			ImGui::Checkbox("Activate Hunters", &addHunters);
		}
		
		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
		style.Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1.00f);
		if (ImGui::Button("Reset"))
		{
			Setup();
		}

		//ImGui::SameLine();
		//ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
private:
	int w_width;
	int w_height;
	int nrBoids;
	int nrHunters;

	std::vector<Boid> boids;
	std::vector<glm::vec4> ppColor;
	std::vector<float> ppSeparationForce;

	std::vector<glm::vec2> ppAvoidance;

	std::vector<Point> points;
	std::vector<glm::vec2> velocities;
	std::vector<glm::vec2> accelerations[2];
	int frameIndex = 0;

	enum Behavior
	{
		Boid = 0,
		Leader = 1,
		Hunter = 2,
		Undefined = 3
	};

	struct BoidData
	{
		Behavior behaviorType;
		int arrayIndex;
	};


	BoidData& getBoidData(std::any b)
	{
		return std::any_cast<BoidData&>(b);
	}

	std::vector<float> ppRadius;

	float queryRadius;

	float alignmentRadius;
	float cohesionRadius;
	float separationRadius;

	// max values
	float alignmentMaxForce;
	float alignmentMaxSpeed;

	float cohesionMaxForce;
	float cohesionMaxSpeed;	
	
	float separationMaxForce;
	float separationMaxSpeed;

	int treeLevelCapacity;

	bool alignmentOn = false;
	bool cohesionOn = false;
	bool separationOn = false;

	bool quadTree = false;
	bool drawQuadTree = false;
	bool addHunters = false;

	GLFWwindow* window;
	QuadTree* qt;

	// opengl and draw
	GLuint vboPositions;
	GLuint vboColors;
	GLuint vboLines;
	GLuint vboFoundPoints;

	std::vector<glm::vec2> positions;
	std::vector<glm::vec2> lines;
	std::vector<glm::vec2> rangeLines;
	glm::mat4 mvp;
	GLuint mvpId;

	GLuint program;
};
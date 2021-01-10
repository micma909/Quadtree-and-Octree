#pragma once
#include <iostream>
#include "QuadTree.h"
#include "Util.h"

class QTCollisions
{
public:
    QTCollisions(GLFWwindow* window, int width, int height, int pointCount) : w_width(width)
		, w_height(height)
		, nrPoints(pointCount)
        , window(window)
	{}

	void Setup()
	{
        this->treeLevelCapacity = 1;
        Rectangle boundary(w_width / 2, w_height / 2, w_width / 2, w_height / 2);

        qt = new QuadTree(boundary, this->treeLevelCapacity);

        velocities.resize(nrPoints);
        radiuses.resize(nrPoints);
        speeds.resize(nrPoints);
        colors.resize(nrPoints);

        for (int i = 0; i < nrPoints; i++)
        {
            positions.push_back({ RandomFloat(0, w_width), RandomFloat(0, w_height) / 32 + (w_height / 2) });
            velocities[i] = glm::vec2(-1,0);
            radiuses[i] = RandomFloat(4, 6);
            speeds[i] = RandomFloat(1, 2);
            colors[i] = glm::vec4(1.f,0.8f,0.f,1.f);
        
            qt->insert(&positions[i]);
        }
        this->oldState_left = GLFW_RELEASE;
        this->oldState_right = GLFW_RELEASE;
        this->oldFlockingState = GLFW_RELEASE;
        this->oldWrapState = GLFW_RELEASE;

        this->xDrag = 0;
        this->yDrag = 0;
        this->xw = 0;
        this->yh = 0;

        this->avgCollisions = 0;
        this->countFFF = 0;

        this->flocking = false;
        this->press = false;
        this->doSearch = false;
        this->runNaive = false;
        this->runQuad = false;
        this->wrap = true;
        this->pauseTime = false;
	}

    void Run()
    {
        UserInputs();
        if (!pauseTime)
        {
            MovePoints();
            NaiveCollision();
            QuadTreeCollision();
        }
        DebugDraw();
    }

    void UserInputs()
    {
        // left mouse
        int newState_left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (newState_left == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            positions.push_back(Point(xpos, w_height - ypos));
            //positions.push_back({ rand() % w_width, rand() % w_height / 32 + (w_height / 2) });
            velocities.push_back(glm::vec2(-1, 0));
            radiuses.push_back(4.0f);//RandomFloat(2, 10);
            speeds.push_back(RandomFloat(1, 2));
            colors.push_back(glm::vec4(0.f, 1.0f, 1.f, 1.f));

            clicks++;

        }
        oldState_left = newState_left;

        // right mouse
        int newState_right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (newState_right == GLFW_PRESS)
        {
            if (!press)
            {
                foundPoints.clear();
                glfwGetCursorPos(window, &xDrag, &yDrag);
                yDrag = w_height - yDrag;
                press = true;
            }

            double xp, yp;
            glfwGetCursorPos(window, &xp, &yp);
            yp = w_height - yp;

            xw = std::abs(xDrag - xp);
            yh = std::abs(yDrag - yp);
        }
        if (newState_right == GLFW_RELEASE && oldState_right == GLFW_PRESS)
        {
            press = false;
            doSearch = true;
        }

        oldState_right = newState_right;

        int capacityState = glfwGetKey(window, GLFW_KEY_KP_ADD);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity++;
        }

        capacityState = glfwGetKey(window, GLFW_KEY_KP_SUBTRACT);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity--;
            if (treeLevelCapacity <= 0)
                treeLevelCapacity = 1;
        }

        int flockingState = glfwGetKey(window, GLFW_KEY_F);
        if (flockingState == GLFW_RELEASE && oldFlockingState == GLFW_PRESS)
        {
            flocking = !flocking;
        }
        oldFlockingState = flockingState;

        int wrapState = glfwGetKey(window, GLFW_KEY_W);
        if (wrapState == GLFW_RELEASE && oldWrapState == GLFW_PRESS)
        {
            wrap = !wrap;
        }
        oldWrapState = wrapState;

        int timeState = glfwGetKey(window, GLFW_KEY_SPACE);
        if (timeState == GLFW_RELEASE && oldTimeState == GLFW_PRESS)
        {
            pauseTime = !pauseTime;
            countFFF = 0;
        }
        oldTimeState = timeState;

        int state = glfwGetKey(window, GLFW_KEY_1);
        if (state == GLFW_PRESS)
        {
            runNaive = false;
            runQuad = false;
        }
        state = glfwGetKey(window, GLFW_KEY_2);
        if (state == GLFW_PRESS)
        {
            runNaive = true;
            runQuad = false;
        }
        state = glfwGetKey(window, GLFW_KEY_3);
        if (state == GLFW_PRESS)
        {
            runQuad = true;
            runNaive = false;
        }
    }

    void MovePoints()
    {
        qt->clear(treeLevelCapacity);

        for (int i = 0; i < positions.size(); i++)
        {       
            positions[i].pos += velocities[i] * speeds[i];
            borderCheck(&positions[i].pos, &velocities[i], w_width, w_height, wrap);

            bool success = qt->insert(&positions[i]);
            if (!success)
            {
                failures.push_back(positions[i]);
            }
        }
    }

    void NaiveCollision()
    {
     
        if (runNaive)
        {
            nrOfChecks = 0;
            // check collision naive
            for (int i = 0; i < positions.size(); i++)
            {
                Point* p1 = &positions[i];

                colors[i].g += 0.05f;
                colors[i].g = std::min(0.8f, colors[i].g);

                for (int j = 0; j < positions.size(); j++)
                {
                    nrOfChecks++;

                    Point* p2 = &positions[j];

                    float dx = p1->pos.x - p2->pos.x;
                    float dy = p1->pos.y - p2->pos.y;

                    float distance = sqrt(dx * dx + dy * dy);

                    if (i != j && distance * 2.0f < radiuses[i] + radiuses[j])
                    {
                        velocities[i].x =  dx * 0.1f * speeds[i];
                        velocities[i].y =  dy * 0.1f * speeds[i];
                        velocities[j].x = -dx * 0.1f * speeds[j];
                        velocities[j].y = -dy * 0.1f * speeds[j];

                        colors[i].g = 0.0f;
                        colors[j].g = 0.0f;
                    }
                }
            }
        }
    }
    
    void QuadTreeCollision()
    {
        if (runQuad)
        {
            nrOfChecks = 0;
            std::vector<Point*> collidable;
            for (int i = 1; i < positions.size(); i++)
            {
                Point* p1 = &positions[i];
                auto radius = radiuses[i];
                Rectangle pBounds(p1->pos.x, p1->pos.y, radius * 0.5, radius * 0.5);

                qt->query(pBounds, &collidable);

                colors[i].g += 0.05f;
                colors[i].g = std::min(0.8f, colors[i].g);

                for (int j = 0; j < collidable.size(); j++)
                {
                    nrOfChecks++;
              
                    glm::vec2 diff = p1->pos - collidable[j]->pos;
                    float distance = glm::length(diff);

                    if (distance == 0)
                        continue;

                    radius += radiuses[j];
                    if (distance * 2.0f < radius)
                    {
                        velocities[i] = diff * 0.1f * speeds[i];
                        velocities[j] = diff * 0.1f * speeds[j];
                       

                        colors[i].g = 0.0f;
                        colors[j].g = 0.0f;

                        break;
                    }
                }
                collidable.clear();
            }
        }
    }

    void DebugDraw()
    {
        // display avg number of queries
        if (!pauseTime)
        {
            avgCollisions += nrOfChecks;
            countFFF++;
            if (countFFF >= 10)
            {
                countFFF = 0;
                std::cout << avgCollisions / 10 << std::endl;
                avgCollisions = 0;
            }
        }
      

        for (int k = 0; k < failures.size(); k++)
        {
            Rectangle fail(failures[k].pos.x, failures[k].pos.y, 5, 5);
            drawRange(fail, 1, 1, 1, 1);
        }

        Rectangle search(xDrag, yDrag, xw, yh);

       // if (doSearch)
        {
            qt->query(search, &foundPoints);
            g_count = 0;
            doSearch = false;
        }

        if (flocking || runQuad)
            qt->drawGrid();
        drawPoints(positions, velocities, radiuses, colors);

        // searchrange
        drawRange(search, 0.f, 1.f, 0.f, 1.0f);
        drawQuad(search, 0.f, 0.f, 0.f, 0.55f);
        drawPoints(&foundPoints, 0.f, 1.f, 1.f, 1.0f);
        foundPoints.clear();
    }

private:
	int w_width;
	int w_height;
	int nrPoints;

    std::vector<Point> positions;
    std::vector<glm::vec2> velocities;
    std::vector<float> radiuses;
    std::vector<float> speeds;
    std::vector<glm::vec4> colors;

    std::vector<Point*> foundPoints{};
    std::vector<Point> failures{};

    int clicks;
    int oldState_left;
    int oldState_right;
    int oldFlockingState;
    int oldWrapState;
    int oldTimeState;


    double xDrag, yDrag;
    double xw, yh;

    int avgCollisions;
    int countFFF;
    int nrOfChecks;

    bool flocking;
    bool press;
    bool doSearch;
    bool runNaive;
    bool runQuad;
    bool wrap;
    bool pauseTime;

    int treeLevelCapacity;

    GLFWwindow* window;
    QuadTree* qt;
};
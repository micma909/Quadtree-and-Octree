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

        for (int i = 0; i < nrPoints; i++)
        {
            allPoints.push_back({ 0,0,0,0,0 });
            allPoints[i].x = rand() % w_width;// +(w_width / 4);
            allPoints[i].y = rand() % w_height / 32 + (w_height / 2);
            allPoints[i].vx = 1.0f;// RandomFloat(-1,1);
            allPoints[i].vy = 0.0f; //RandomFloat(-1, 1);
            allPoints[i].r = RandomFloat(2, 10);
            allPoints[i].speed = RandomFloat(1, 2);
            allPoints[i].color = 0.0f;

            qt->insert(&allPoints[i]);
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

            allPoints.push_back(Point(xpos, w_height - ypos, 0, 0, 2.0f));
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

        for (int i = 0; i < allPoints.size(); i++)
        {
            allPoints[i].collision = false;
       
            allPoints[i].x += allPoints[i].vx * allPoints[i].speed;
            allPoints[i].y += allPoints[i].vy * allPoints[i].speed;

            borderCheck(&allPoints[i], w_width, w_height, wrap);

            bool success = qt->insert(&allPoints[i]);
            if (!success)
            {
                failures.push_back(allPoints[i]);
            }
        }
    }

    void NaiveCollision()
    {
        nrOfChecks = 0;
        if (runNaive)
        {
            // check collision naive
            for (int i = 0; i < allPoints.size(); i++)
            {
                Point* p1 = &allPoints[i];
                p1->color -= 0.05f;
                for (int j = 0; j < allPoints.size(); j++)
                {
                    nrOfChecks++;

                    Point* p2 = &allPoints[j];

                    float dx = p1->x - p2->x;
                    float dy = p1->y - p2->y;

                    float distance = sqrt(dx * dx + dy * dy);
                    p1->collision = false;

                    if (i != j && distance * 2.0f < p1->r + p2->r)
                    {
                        p1->vx = dx * 0.1f * p1->speed;
                        p1->vy = dy * 0.1f * p1->speed;
                        p2->vx = -dx * 0.1f * p2->speed;
                        p2->vy = -dy * 0.1f * p2->speed;

                        p1->color = 1.0f;
                        p2->color = 1.0f;

                        p1->collision = true;
                        break;
                    }
                }
            }
        }
    }
    
    void QuadTreeCollision()
    {
        if (runQuad)
        {
            std::vector<Point*> collidable;
            for (int i = 1; i < allPoints.size(); i++)
            {
                Point* p1 = &allPoints[i];
                Rectangle pBounds(p1->x, p1->y, p1->r * 0.5, p1->r * 0.5);

                p1->color -= 0.05f;

                qt->query(pBounds, &collidable);

                for (int j = 0; j < collidable.size(); j++)
                {
                    nrOfChecks++;
                    Point* p2 = collidable[j];

                    float dx = p1->x - p2->x;
                    float dy = p1->y - p2->y;

                    float distance = sqrt(dx * dx + dy * dy);

                    if (distance == 0)
                        continue;

                    p1->collision = false;

                    if (distance * 2.0f < p1->r + p2->r)
                    {
                        p1->vx = dx * 0.1f * p1->speed;
                        p1->vy = dy * 0.1f * p1->speed;
                        p2->vx = -dx * 0.1f * p2->speed;
                        p2->vy = -dy * 0.1f * p2->speed;

                        p1->color = 1.0f;
                        p2->color = 1.0f;

                        p2->collision = true;
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
                std::cout << avgCollisions / 100 << std::endl;
                avgCollisions = 0;
            }
        }
      

        for (int k = 0; k < failures.size(); k++)
        {
            Rectangle fail(failures[k].x, failures[k].y, 5, 5);
            drawRange(fail, 1, 1, 1, 1);
        }

        Rectangle search(xDrag, yDrag, xw, yh);

       // if (doSearch)
        {
            qt->query(search, &foundPoints);
            //std::cout << "searched : " << g_count << " times" << std::endl;
            g_count = 0;
            doSearch = false;
        }

        if (flocking || runQuad)
            qt->drawGrid();
        drawPoints(&allPoints, 1.0f, 0.8f, 0.0f, 1.0f);

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

    std::vector<Point> allPoints;
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
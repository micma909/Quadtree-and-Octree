#include <iostream>

#include <random>
#include <sstream>


#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>
using namespace glm;

#include "QuadTree.h"
#include "Flocking.h"

float RandomFloat(float min, float max)
{
    assert(max > min);
    float random = ((float)rand()) / (float)RAND_MAX;
    float range = max - min;
    return (random * range) + min;
}

int main() 
{
    int w_width = 1200;
    int w_height = 1200;
    int nrPoints = 100;
    float particleRadius = 4.0f;

    int treeLevelCapacity = 1;

    Point randomFlockingPoint(500, 500, 0, 0, 0);

    Rectangle boundary(600, 600, 600, 600);
    Rectangle innerMargin(600, 600, 590, 590);

    QuadTree* qt = new QuadTree(boundary, treeLevelCapacity);


    std::default_random_engine generator;
    std::normal_distribution<float> distribution(400, 200.0);

    std::vector<Point> allPoints(nrPoints, {0,0,0,0, particleRadius});
 
    for (int i = 0; i < nrPoints; i++)
    {
        allPoints[i].x = rand() % w_width/4 + (w_width / 4);
        allPoints[i].y = rand() % w_height/4 + (w_width / 4);
        allPoints[i].vx = RandomFloat(-1,0);
        allPoints[i].vy = RandomFloat(-1,0);
        allPoints[i].r = RandomFloat(2, 10);
        allPoints[i].speed = RandomFloat(2, 4);
        
        qt->insert(&allPoints[i]);
       
    }
    qt->clear();
  //  allPoints.push_back(Point(0, 0));

    std::cout << "Total inserted : " << qt->totalInserted() << std::endl;

    for (int i = 0; i < nrPoints; i++)
    {
        qt->insert(&allPoints[i]);
    }

    std::cout << "Total inserted : " << qt->totalInserted() << std::endl;

    // Initialise GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    // Open a window and create its OpenGL context
    GLFWwindow* window = glfwCreateWindow(w_width, w_height, "QuadTree", NULL, NULL);
    glfwMakeContextCurrent(window); // Initialize GLEW

    glViewport(0.f, 0.f, w_width, w_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w_width, 0, w_height, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glewExperimental = true;

    std::vector<Point*> foundPoints{};
    static int clicks = 0;

    static int oldState_left = GLFW_RELEASE;

    double xDrag = 0, yDrag = 0;
    double xw = 0, yh = 0;
    static int oldState_right = GLFW_RELEASE;
    bool press = false;
    bool doSearch = false;
    std::vector<Point> failures{};

    double previousTime = glfwGetTime();
    int frameCount = 0;

    bool runNaive = false;
    bool runQuad = false;

    int avgCollisions = 0;
    int countFFF = 0;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
       
        // left mouse
        int newState_left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
       // if (newState_left == GLFW_RELEASE && oldState_left == GLFW_PRESS)
        if (newState_left == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
        
           // Point aPoint = Point(xpos, w_height - ypos);
            allPoints.push_back(Point(xpos, w_height - ypos, RandomFloat(-1, 1), RandomFloat(-1, 1), particleRadius));
           // bool success = qt->insert(&allPoints[allPoints.size() - 1]);
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

        int capacityState = glfwGetKey(window, GLFW_KEY_9);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity++;
        }

        capacityState = glfwGetKey(window, GLFW_KEY_0);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity--;
            if (treeLevelCapacity <= 0)
                treeLevelCapacity = 1;
        }

        qt->clear(treeLevelCapacity);

        for (int i = 0; i < allPoints.size(); i++)
        {
            allPoints[i].collision = false;

            allPoints[i].x += allPoints[i].vx * allPoints[i].speed;
            allPoints[i].y += allPoints[i].vy * allPoints[i].speed;

            if (!innerMargin.contains(allPoints[i]))
            {
                allPoints[i].vx *= -1;
                allPoints[i].vy *= -1;
                allPoints[i].x += allPoints[i].vx * 2.0f;
                allPoints[i].y += allPoints[i].vy * 2.0f;
            }

            bool success = qt->insert(&allPoints[i]);
            if (!success)
            {
                failures.push_back(allPoints[i]);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
       

        for (int k = 0; k < failures.size(); k++)
        {
            Rectangle fail(failures[k].x, failures[k].y, 5, 5);
            drawRange(fail, 1, 1, 1, 1);
        }

        Rectangle search(xDrag, yDrag, xw, yh);

        drawRange(search, 0.f, 1.f, 0.f, 1.0f);
        drawQuad(search, 0.f, 0.f, 0.f, 0.55f);

        int total = qt->totalInserted();
        int size = allPoints.size();
        if(size != qt->totalInserted())
            std::cout << size << " -> "<< total << std::endl;

        if (doSearch)
        {
            qt->query(search, &foundPoints);
            std::cout << "searched : " << g_count << " times" << std::endl;
            g_count = 0;
            doSearch = false;
        }

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

        int nrOfChecks = 0;

        if (runNaive)
        {
            // check collision naive
            for (int i = 0; i < allPoints.size(); i++)
            {
                Point* p1 = &allPoints[i];
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
                        p1->vx =  dx * 0.1f * p1->speed;
                        p1->vy =  dy * 0.1f * p1->speed;
                        p2->vx = -dx * 0.1f * p2->speed;
                        p2->vy = -dy * 0.1f * p2->speed;

                        p1->collision = true;
                        break;
                    }
                }
            }
        }
        
        if (runQuad)
        {
            std::vector<Point*> collidable;
            for (int i = 1; i < allPoints.size(); i++)
            {
                Point* p1 = &allPoints[i];
                Rectangle pBounds(p1->x, p1->y, p1->r*0.5, p1->r * 0.5);

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
                        p1->vx =  dx * 0.1f*p1->speed;
                        p1->vy =  dy * 0.1f*p1->speed;
                        p2->vx = -dx * 0.1f*p2->speed;
                        p2->vy = -dy * 0.1f*p2->speed;

                        p2->collision = true;
                        break;
                    }
                }
                collidable.clear();
            }
            qt->draw();
        }

       
        avgCollisions += nrOfChecks;
        
        countFFF++;
        if (countFFF >= 100)
        {
            countFFF = 0;
            std::cout << avgCollisions/100 << std::endl;
            avgCollisions = 0;

            randomFlockingPoint.x = rand() % w_width / 2 + (w_width / 4);
            randomFlockingPoint.y = rand() % w_height / 2 + w_height / 4;


            glColor4f(1, 0, 1, 1);
            GLfloat pointVert[] = { randomFlockingPoint.x, randomFlockingPoint.y };
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, pointVert);
            glDrawArrays(GL_POINTS, 0, 1);
            glDisableClientState(GL_VERTEX_ARRAY);
        }

        // Measure speed
        double currentTime = glfwGetTime();
        frameCount++;
        // If a second has passed.
        if (currentTime - previousTime >= 1.0)
        {
            // Display the frame count here any way you want.
            std::stringstream ss;
            ss  << frameCount << " FPS";

            glfwSetWindowTitle(window, ss.str().c_str());

            frameCount = 0;
            previousTime = currentTime;
        }


        drawPoints(&allPoints, 1.0f, 0.8f, 0.0f, 1.0f);
        drawPoints(&foundPoints, 0.f, 1.f, 1.f, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();


    }
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    return 0;
}

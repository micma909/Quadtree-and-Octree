#pragma once
#include "QuadTree.h"
#include <sstream>

float RandomFloat(float min, float max)
{
    assert(max > min);
    float random = ((float)rand()) / (float)RAND_MAX;
    float range = max - min;
    return (random * range) + min;
}

void displayFPS(GLFWwindow* window, int& frameCount, double& previousTime)
{
    double currentTime = glfwGetTime();
    frameCount++;
    // If a second has passed.
    if (currentTime - previousTime >= 1.0)
    {
        // Display the frame count here any way you want.
        std::stringstream ss;
        ss << frameCount << " FPS";

        glfwSetWindowTitle(window, ss.str().c_str());

        frameCount = 0;
        previousTime = currentTime;
    }
}

// deprecated 
void borderCheck(Point* p, int w_width, int w_height, bool wrap)
{
    if (wrap)
    {
        float margin = 10.0f;
        if (p->x < margin) p->x = w_width - margin;
        if (p->y < margin) p->y = w_height - margin;
        if (p->x > w_width - margin)  p->x = margin;
        if (p->y > w_height - margin) p->y = margin;
    }
    else
    {
        Rectangle innerMargin(500, 500, 490, 490);
        if (!innerMargin.contains(*p))
        {
            p->vx *= -1;
            p->vy *= -1;
            p->x += p->vx * 2.0f;
            p->y += p->vy * 2.0f;
        }
    }
}

void borderCheck(glm::vec2* point, int w_width, int w_height, bool wrap = true) // wrap true for now
{
    if (wrap)
    {
        float margin = 10.0f;
        if (point->x < margin) point->x = w_width - margin;
        if (point->y < margin) point->y = w_height - margin;
        if (point->x > w_width - margin)  point->x = margin;
        if (point->y > w_height - margin) point->y = margin;
    }
}

// move this to util at some point
static float distance(glm::vec2* p1, glm::vec2* p2)
{
    glm::vec2 diff = *p1 - *p2;
    return glm::sqrt(std::pow(diff.x, 2) + std::pow(diff.y, 2));
}
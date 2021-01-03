#include <iostream>

#include <random>

#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>
using namespace glm;

#include "QuadTree.h"
//#include "Flocking.h"
#include "Util.h"

#include "QuadTreeCollisions.h"
#include "QuadTreeFlocking.h"

const int w_width = 1000;
const int w_height = 1000;
const int nrPoints = 100;

//#define QUADTREE_COLLISIONS
#define QUADTREE_FLOCKING

int main() 
{  
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

#ifdef QUADTREE_FLOCKING
    QTFlocking quadTreeFlocking(window, w_width, w_height, nrPoints);
    quadTreeFlocking.Setup();
#endif 

#ifdef QUADTREE_COLLISIONS
    QTCollisions quadTreeCollisions(window, w_width, w_height, nrPoints);
    quadTreeCollisions.Setup();
#endif
    int frameCount = 0;
    double previousTime = glfwGetTime();

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
        glClear(GL_COLOR_BUFFER_BIT);

#ifdef QUADTREE_FLOCKING
        quadTreeFlocking.Run();
#endif 

#ifdef QUADTREE_COLLISIONS
        quadTreeCollisions.Run();
#endif 

        displayFPS(window, frameCount, previousTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
       // glfwSwapInterval(0);

    }
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    return 0;
}

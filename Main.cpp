#include <iostream>
#include <random>

// imgui 
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


// GL Utils
#include "Shader.h"

// QuadTree
//#include "QuadTree.h"
#include "Util.h"
#include "Geometries.h"

// QuadTree examples
#include "QuadTreeCollisions.h"
#include "QuadTreeFlocking.h"
#include "OctreeTest.h"

static const int w_width = 1000;
static const int w_height = 1000;
static int nrPoints = 500;

//#define QUADTREE_COLLISIONS
//#define QUADTREE_FLOCKING

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

    GLenum err = glewInit();
    if (GLEW_OK != err)
          std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    std::cerr << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

#ifdef QUADTREE_COLLISIONS
    QTCollisions quadTreeCollisions(window, w_width, w_height, nrPoints);
    quadTreeCollisions.Setup();
#endif
#ifdef QUADTREE_FLOCKING
    QTFlocking quadTreeFlocking(window, w_width, w_height, nrPoints);
    quadTreeFlocking.Setup();
#endif

    OctreeTest octTest;
    octTest.Setup(window, w_width, w_height);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.WantCaptureMouse = false;
    io.DisplaySize.x = w_width;
    io.DisplaySize.y = w_height;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //// Setup Platform/Renderer backends
    const char* glsl_version = "#version 420";
    ImGui_ImplOpenGL3_Init(glsl_version);
    // Setup Dear ImGui style

    int frameCount = 0;
    double previousTime = glfwGetTime();
    bool show_demo_window = false;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef QUADTREE_COLLISIONS
        quadTreeCollisions.Run();
#endif

#ifdef QUADTREE_FLOCKING
        quadTreeFlocking.Run();
        quadTreeFlocking.Draw();
        quadTreeFlocking.ImGuiMenu();
#endif
        octTest.Draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        displayFPS(window, frameCount, previousTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
        ImGui::EndFrame();
        //glfwSwapInterval(0);

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
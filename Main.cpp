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
#include "QuadTree.h"
#include "Util.h"
#include "Geometries.h"

// QuadTree examples
#include "QuadTreeCollisions.h"
#include "QuadTreeFlocking.h"

static const int w_width = 1000;
static const int w_height = 1000;
static int nrPoints = 500;

//#define QUADTREE_COLLISIONS
//#define QUADTREE_FLOCKING
int oldMain() 
{ 
#ifdef QUADTREE_FLOCKING
    nrPoints = 500;
#endif 

    // Initialise GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
  
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    // Open a window and create its OpenGL context
    GLFWwindow* window = glfwCreateWindow(w_width+500, w_height, "QuadTree", NULL, NULL);
    glfwMakeContextCurrent(window); // Initialize GLEW

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }
    std::cerr << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

    //// newer opengl ///
    Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
    GLuint program = shader.createProgram();

    glViewport(0.f, 0.f, w_width, w_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w_width, 0, w_height, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);
    // Setup Dear ImGui style

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
    bool show_demo_window = false;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
       ImGui_ImplOpenGL3_NewFrame();
       ImGui_ImplGlfw_NewFrame();
       ImGui::NewFrame();

       //ImGui::ShowDemoWindow(&show_demo_window);
       glClear(GL_COLOR_BUFFER_BIT);

#ifdef QUADTREE_FLOCKING
        quadTreeFlocking.Run();
        quadTreeFlocking.ImGuiMenu();
#endif 

#ifdef QUADTREE_COLLISIONS
        quadTreeCollisions.Run();
#endif 

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        displayFPS(window, frameCount, previousTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
        ImGui::EndFrame();
        //glfwSwapInterval(0);

    }
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

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

    //QTCollisions quadTreeCollisions(window, w_width, w_height, nrPoints);
    //quadTreeCollisions.Setup();

    QTFlocking quadTreeFlocking(window, w_width, w_height, nrPoints);
    quadTreeFlocking.Setup();


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

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);

       // quadTreeCollisions.Run();
        quadTreeFlocking.Run();
        quadTreeFlocking.Draw();
        quadTreeFlocking.ImGuiMenu();

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
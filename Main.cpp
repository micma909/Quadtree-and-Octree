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
static int nrPoints = 100;

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


    QTCollisions quadTreeCollisions(window, w_width, w_height, nrPoints);
    quadTreeCollisions.Setup();

    GLenum err = glewInit();
    if (GLEW_OK != err)
          std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    std::cerr << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

    //// newer opengl ///
    Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
    GLuint program = shader.createProgram();

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

    std::vector<glm::vec2> points = quadTreeCollisions.GetQTPoints();

    std::vector<glm::vec2> lines;
    quadTreeCollisions.GetQuadTree()->GetBoundsLineSegments(lines);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
        
    GLuint vboPoints;
    glGenBuffers(1, &vboPoints);
    glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * points.size(), &points[0].x, GL_DYNAMIC_DRAW);

    glm::mat4 Projection = glm::ortho(0.f, static_cast<float>(w_width),  0.f, static_cast<float>(w_height), 0.f, 100.f);
    glm::mat4 View = glm::lookAt
    (
        glm::vec3(0, 0, -1), // -1 in Z
        glm::vec3(0, 0, 0), // look at origin
        glm::vec3(0, 1, 0)  // y-ax is up
    );
    
    glm::mat4 Model = glm::mat4(1.f);
    glm::mat4 mvp = Projection; //* View * Model;

    GLuint mvpId = glGetUniformLocation(program, "MVP");

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    do {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_CULL_FACE);

        glUseProgram(program);
        glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);
        
        //ImGui::ShowDemoWindow(&show_demo_window);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glPointSize(3);
        glDrawArrays(GL_POINTS, 0, points.size());

        quadTreeCollisions.Run();
        
        std::cout << nrPoints << " " << quadTreeCollisions.GetQuadTree()->totalInserted() << std::endl;

        // cleanup & reuppload 
        points.clear();

        points = quadTreeCollisions.GetQTPoints();
        glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * points.size(), &points[0].x, GL_STATIC_DRAW);

        lines.clear();
        quadTreeCollisions.GetQuadTree()->GetBoundsLineSegments(lines);
        
        GLuint vboLines;
        glGenBuffers(1, &vboLines);
        glBindBuffer(GL_ARRAY_BUFFER, vboLines);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * lines.size(), &lines[0].x, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, vboLines);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_LINES, 0, lines.size());

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
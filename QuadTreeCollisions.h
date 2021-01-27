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

        points.resize(nrPoints);
        velocities.resize(nrPoints);
        radiuses.resize(nrPoints);
        speeds.resize(nrPoints);
        colors.resize(nrPoints);

        for (int i = 0; i < nrPoints; i++)
        {
            points[i] = { RandomFloat(0, w_width), RandomFloat(0, w_height) / 32 + (w_height / 2), i };
            velocities[i] = glm::vec2(-2.5f,0);
            radiuses[i] = 10.f;// RandomFloat(4, 6);
            speeds[i] = RandomFloat(0.2f, 0.5f);
            colors[i] = glm::vec4(1,0.8f,0.f,1.f);
        
            qt->insert(&points[i]);
        }
        this->oldState_left = GLFW_RELEASE;
        this->oldState_right = GLFW_RELEASE;
        this->oldWrapState = GLFW_RELEASE;

        this->xDrag = 0;
        this->yDrag = 0;
        this->xw = 0;
        this->yh = 0;

        this->avgCollisions = 0;
        this->countFFF = 0;

        this->press = false;
        this->doSearch = false;
        this->runNaive = false;
        this->runQuad = false;
        this->wrap = true;
        this->pauseTime = false;

        // opengl stuff
        Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
        program = shader.createProgram();

        positions = GetQTPoints();

        glGenBuffers(1, &vboPositions);
        glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &vboColors);
        glBindBuffer(GL_ARRAY_BUFFER, vboColors);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

        std::vector<glm::vec2> lines;
        qt->GetBoundsLineSegments(lines);

        mvp = glm::ortho(0.f, static_cast<float>(w_width), 0.f, static_cast<float>(w_height), 0.f, 100.f);
        mvpId = glGetUniformLocation(program, "MVP");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
        Draw();
    }
    void Draw()
    {
        glUseProgram(program);
        glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vboColors);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        positions = GetQTPoints();

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
        glPointSize(3);
        glDrawArrays(GL_POINTS, 0, positions.size());

        positions.clear();
        lines.clear();
        qt->GetBoundsLineSegments(lines);

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

        Rectangle search(xDrag, yDrag, xw, yh);
      
       // if (doSearch)
        {
            std::vector<Point*> foundPoints;
            qt->query(search, &foundPoints);
            g_count = 0;
            doSearch = false;

            for (int i = 0; i < foundPoints.size(); i++)
            {
                int index = std::any_cast<int&>(foundPoints[i]->data);
                foundPositions.push_back(foundPoints[i]->pos);
            }
        }

        if (foundPositions.size())
        {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * foundPositions.size(), &foundPositions[0].x, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);

            glPointSize(10);
            glDrawArrays(GL_POINTS, 0, foundPositions.size());
            glDisableVertexAttribArray(0);
        }
        foundPositions.clear();

        int x = search.x;
        int y = search.y;
        int w = search.w;
        int h = search.h;
        rangeLines.push_back({ x - w, y - h });
        rangeLines.push_back({ x - w, y + h });
        rangeLines.push_back({ x + w, y + h });
        rangeLines.push_back({ x + w, y - h });
        rangeLines.push_back({ x - w, y - h });

        glEnableVertexAttribArray(0);
        glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);

        GLuint vboRange;
        glGenBuffers(1, &vboRange);
        glBindBuffer(GL_ARRAY_BUFFER, vboRange);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * rangeLines.size(), &rangeLines[0].x, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vboRange);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glDrawArrays(GL_LINE_STRIP, 0, rangeLines.size());
        glDisableVertexAttribArray(0);
        rangeLines.clear();
    }


    void UserInputs()
    {
        // left mouse
        int newState_left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (newState_left == GLFW_PRESS)
        {
            // TODO fix this 

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            
            points.push_back(Point(xpos, w_height - ypos));
            velocities.push_back(glm::vec2(-1, 0));
            radiuses.push_back(4.0f);//RandomFloat(2, 10);
            speeds.push_back(RandomFloat(1, 2));
            colors.push_back(glm::vec4(1, 0.8f, 0.f, 1.f));
            
            clicks++;

        }
        oldState_left = newState_left;

        // right mouse
        int newState_right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (newState_right == GLFW_PRESS)
        {
            if (!press)
            {
                rangeLines.clear();
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

        for (int i = 0; i < points.size(); i++)
        {       
            points[i].pos += velocities[i] * speeds[i];
            borderCheck(&points[i].pos, &velocities[i], w_width, w_height, wrap);

            bool success = qt->insert(&points[i]);
            if (!success)
            {
                failures.push_back(points[i]);
            }
        }
    }

    void NaiveCollision()
    {
        if (runNaive)
        {
            nrOfChecks = 0;
            // check collision naive
            for (int i = 0; i < points.size(); i++)
            {
                Point* p1 = &points[i];

                colors[i].g += 0.05f;
                colors[i].g = std::min(0.8f, colors[i].g);

                for (int j = 0; j < points.size(); j++)
                {
                    nrOfChecks++;

                    Point* p2 = &points[j];

                    float dx = p1->pos.x - p2->pos.x;
                    float dy = p1->pos.y - p2->pos.y;

                    float distance = sqrt(dx * dx + dy * dy);

                    if (i != j && distance * 2.0f < radiuses[i] + radiuses[j])
                    {
                        velocities[i].x =  dx * speeds[i];
                        velocities[i].y =  dy * speeds[i];
                        velocities[j].x = -dx * speeds[j];
                        velocities[j].y = -dy * speeds[j];

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
            for (int i = 0; i < points.size(); i++)
            {
                Point* p1 = &points[i];
                auto radius = radiuses[i];
                Rectangle pBounds(p1->pos.x, p1->pos.y, radius * 0.5f, radius * 0.5f);

                qt->query(pBounds, &collidable);

                colors[i].g += 0.05f;
                colors[i].g = std::min(0.8f, colors[i].g);

                for (int j = 0; j < collidable.size(); j++)
                {
                    nrOfChecks++;
              
                    glm::vec2 diff = p1->pos - collidable[j]->pos;
                    float distance = glm::length(diff);

                    if (distance < 0.001f)
                        continue;

                    radius += radiuses[j];
                    if (distance * 2.0f < radius)
                    {
                        velocities[i] = diff * speeds[i];                       
                        colors[i].g = 0.0f;

                        break;
                    }
                }
                collidable.clear();
            }
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

    QuadTree* GetQuadTree()
    {
        return qt;
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
    }

private:
    std::vector<glm::vec2> debugCollidable;

	int w_width;
	int w_height;
	int nrPoints;

    std::vector<Point> points;
    std::vector<glm::vec2> velocities;
    std::vector<glm::vec4> colors;
    std::vector<float> radiuses;
    std::vector<float> speeds;

    // mostly for debugging
    std::vector<glm::vec2> foundPositions;
    std::vector<Point> failures{};

    // glfw input 
    int clicks;
    int oldState_left;
    int oldState_right;
    int oldWrapState;
    int oldTimeState;

    double xDrag, yDrag;
    double xw, yh;

    int avgCollisions;
    int countFFF;
    int nrOfChecks;

    bool press;
    bool doSearch;
    bool runNaive;
    bool runQuad;
    bool wrap;
    bool pauseTime;

    int treeLevelCapacity;

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
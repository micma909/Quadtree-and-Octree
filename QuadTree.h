#pragma once
#include <vector>
#include <cassert>
#include <unordered_map>
#include <any>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
using namespace glm;

struct Point
{
	Point() : pos(0,0), data(nullptr){}
	Point(float x, float y, std::any d = nullptr) : pos(x, y), data(d){}
	glm::vec2 pos;
	std::any data;
};

class Rectangle
{
public:
	Rectangle(float x, float y, float w, float h) : x(x), y(y), w(w), h(h)
	{}
	float x;
	float y;
	float w; 
	float h;

	bool contains(glm::vec2 p, float eps = 0.f)
	{
		return (p.x >= x + eps - w && p.x <= x + eps + w && 
				p.y >= y + eps - h && p.y <= y + eps + h); 
	}

	bool intersects(Rectangle range)
	{
		return !(range.x - range.w > x + w ||
				 range.x + range.w < x - w ||
				 range.y - range.h > y + h ||
				 range.y + range.h < y - h);
	}
};

static void drawBounds(Rectangle b)
{
	GLfloat lineVertices[] =
	{
		b.x - b.w, b.y, 0,
		b.x + b.w, b.y, 0,
		b.x, b.y + b.h, 0,
		b.x, b.y - b.h, 0
	};
	// Draw
	glColor4f(1.f, 1.f, 1.f, 0.2f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, lineVertices);
	glDrawArrays(GL_LINES, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void drawPoints(std::vector<Point>& points, 
	std::vector<glm::vec2>& velocities, 
	std::vector<float>& radiuses,
	std::vector<glm::vec4>& colors)
{
	for (int i = 0; i < points.size(); i++)
	{
		glPointSize(radiuses[i]);
		glColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
		
		GLfloat pointVert[] = { points[i].pos.x, points[i].pos.y };
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, pointVert);
		glDrawArrays(GL_POINTS, 0, 1);
		glDisableClientState(GL_VERTEX_ARRAY);

		glm::vec2 vel = glm::normalize(velocities[i]);
		GLfloat lineVertices[] =
		{
			points[i].pos.x, points[i].pos.y, 0,
			points[i].pos.x + vel.x*5.f, points[i].pos.y + vel.y* 5.f, 0
		};
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, lineVertices);
		glDrawArrays(GL_LINE_STRIP, 0, 2);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

static void drawPoints(std::vector<Point*>* points, float r, float g, float b, float a)
{
	
	glColor4f(r, g, b, a);
	for (auto& p : *points)
	{
		//glPointSize(p->r);
		glPointSize(2.0f);

		GLfloat pointVert[] = { p->pos.x, p->pos.y };
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, pointVert);
		glDrawArrays(GL_POINTS, 0, 1);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	glPointSize(2);
}

static void drawQuad(Rectangle rec, float r, float g, float b, float a)
{
	int x = rec.x;
	int y = rec.y;
	int w = rec.w;
	int h = rec.h;

	glColor4f(r, g, b, a);

	glBegin(GL_TRIANGLES);
	glVertex2i(x - w, y - h);
	glVertex2i(x + w, y + h);
	glVertex2i(x - w, y + h);
	glVertex2i(x - w, y - h);
	glVertex2i(x + w, y - h);
	glVertex2i(x + w, y + h);
	glEnd();
}

static void drawRange(Rectangle rec, float r, float g, float b, float a)
{
	int x = rec.x;
	int y = rec.y;
	int w = rec.w;
	int h = rec.h;

	GLfloat lineVertices[] =
	{
		x - w, y - h, 0,
		x - w, y + h, 0,
		x + w, y + h, 0,
		x + w, y - h, 0,
		x - w, y - h, 0,
	};
	// Draw
	glColor4f(r, g, b, a);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, lineVertices);
	glDrawArrays(GL_LINE_STRIP, 0, 5);
	glDisableClientState(GL_VERTEX_ARRAY);
}
static int g_count = 0;
class QuadTree
{
public:
	QuadTree(Rectangle boundary, int n) : boundary(boundary), capacity(n), divided(false), level(0),
		north_west(nullptr),
		north_east(nullptr),
		south_west(nullptr),
		south_east(nullptr)
	{}

	void subdivide()
	{
		int x = boundary.x;
		int y = boundary.y;
		int w = boundary.w;
		int h = boundary.h;

		allBounds.push_back(glm::vec2(x - w, y));
		allBounds.push_back(glm::vec2(x + w, y));
		allBounds.push_back(glm::vec2(x, y + h));
		allBounds.push_back(glm::vec2(x, y - h));

		Rectangle ne(x + w / 2, y - h / 2, w / 2,h / 2);
		north_east = new QuadTree(ne, capacity);

		Rectangle nw(x - w / 2, y - h / 2, w / 2, h / 2);
		north_west = new QuadTree(nw, capacity);

		Rectangle se(x + w / 2, y + h / 2, w / 2, h / 2);
		south_east = new QuadTree(se, capacity);

		Rectangle sw(x - w / 2, y + h / 2, w / 2, h / 2);
		south_west = new QuadTree(sw, capacity);

		divided = true;

		north_east->level = this->level + 1;
		north_west->level = this->level + 1;
		south_east->level = this->level + 1;
		south_west->level = this->level + 1;

		delegateToLeafNodes();
		capacity = 0;
	}

	void delegateToLeafNodes()
	{
		if (!divided)
			return;

		int size = m_points.size();
		for (int i = 0; i < size; ++i)
		{
			if (north_west->insert(m_points[i]) ||
				north_east->insert(m_points[i]) ||
				south_west->insert(m_points[i]) ||
				south_east->insert(m_points[i]))
			{
				m_points.erase(m_points.begin() + i);
			}

			if (size != (int)m_points.size())
			{
				--i;
				size = m_points.size();
			}
		}
		//m_points.clear(); // not sure about this?
	}
	

	bool insert(Point* p, float eps = 0.f)
	{
		if(divided && m_points.size() != 0)
			delegateToLeafNodes();

		if (!boundary.contains(p->pos, eps))
			return false;
		
		if (m_points.size() < capacity)
		{
			m_points.push_back(p);
			return true;
		}
		else
		{
			if (!divided)
			{
				subdivide();
			}

			if (north_west->insert(p) ||
				north_east->insert(p) ||
				south_west->insert(p) ||
				south_east->insert(p)) 
				return true;
			else
			{
				m_points.push_back(p);
				return true;
			}
		}
		return false;
	}

	void clear(int newCapacity = 1)
	{
		if (!divided)
		{
			m_points.clear();
		}
		else
		{
			north_west->clear();
			north_east->clear();
			south_west->clear();
			south_east->clear();

			delete north_west;
			delete north_east;
			delete south_west;
			delete south_east;

			divided = false;
			capacity = newCapacity;
		}

		if(allBounds.size())
			allBounds.clear();
	}

	void query(Rectangle range, std::vector<Point*>* result)
	{
		if (!divided && !m_points.size())
			return;
		if(!this->boundary.intersects(range))
			return;
		else
		{
			g_count++; 

			if (divided)
			{
				north_west->query(range, result);
				north_east->query(range, result);
				south_west->query(range, result);
				south_east->query(range, result);
			}

			for (int i = 0; i < m_points.size(); i++)
			{
				if (range.contains(m_points[i]->pos))
				{
 					result->push_back(m_points[i]);
				}
			}
		}
		return;
	}

	void GetBoundsLineSegments(std::vector<glm::vec2>& lines)
	{
		lines = allBounds;
	}

	int totalInserted()
	{
		int total = m_points.size();

		if (north_west != nullptr)
			total += north_west->totalInserted();
		if (north_east != nullptr)
			total += north_east->totalInserted();
		if (south_west != nullptr)
			total += south_west->totalInserted();
		if (south_east != nullptr)
			total += south_east->totalInserted();
		
		return total;
	}

	void drawGrid()
	{
		glEnable(GL_PROGRAM_POINT_SIZE_EXT);

		if (divided)
		{
			drawBounds(boundary);
			north_west->drawGrid();
			north_east->drawGrid();
			south_west->drawGrid();
			south_east->drawGrid();
		}

		if (m_points.size())
		{
			if (divided)
				drawQuad(boundary, 1.0f, 0.0f, 0.0f, 0.3f);
			else
				drawQuad(boundary, 0.0f, 0.8f, 1.0f, 0.05f);
		}
	}

	int level = 0;

private:
	Rectangle boundary;
	int capacity;
	std::vector<Point*> m_points{};

	bool divided = false;

	QuadTree* north_west;
	QuadTree* north_east;
	QuadTree* south_west;
	QuadTree* south_east;

	inline static std::vector<glm::vec2> allBounds{};

};
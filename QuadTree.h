#pragma once
#include <vector>
#include <cassert>

#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>
using namespace glm;

// Notes:



class Point
{
public:
	Point(float x, float y, float vx, float vy, float r) : x(x), y(y), vx(vx), vy(vy), r(r), collision(false)
	{}
	
	float x;
	float y;
	float vx;




	float vy;
	float r;
	bool collision;
	float speed;
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

	bool contains(Point p)
	{
		// is within ? : 
		return (p.x > x - w && // center - width
				p.x < x + w && // center + width
				p.y > y - h && // center - height
				p.y < y + h);  // center + height
		
	    // math:
		// x - w < x < x + w
		// y - h < y < y + h
	}

	bool intersects(Rectangle range)
	{
		return !(range.x - range.w > x + w ||
				 range.x + range.w < x - w ||
				 range.y - range.h > y + h ||
				 range.y + range.h < y - h);
	}

	void drawBounds()
	{
		GLfloat lineVertices[] =
		{
			x - w, y, 0,
			x + w, y, 0,
			x, y + h, 0,
			x, y - h, 0
		};
		// Draw

		glColor4f(1.f, 1.f, 1.f, 0.2f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, lineVertices);
		glDrawArrays(GL_LINES, 0, 4);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

};

static void drawPoints(std::vector<Point>* points, float r, float g, float b, float a)
{
	for (auto& p : *points)
	{
		glPointSize(p.r);
		

		if (p.collision)
			glColor4f(1, 0, 0, 1);
		else
			glColor4f(r, g, b, a);
	

		GLfloat pointVert[] = { p.x, p.y };
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, pointVert);
		glDrawArrays(GL_POINTS, 0, 1);
		glDisableClientState(GL_VERTEX_ARRAY);

		glColor4f(r, g, b, 0.5f);
		
		GLfloat lineVertices[] =
		{
			p.x, p.y, 0,
			p.x + p.vx*10.f, p.y + p.vy * 10.f, 0
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
		glPointSize(p->r);
		if (p->collision)
		{
			glPointSize(10.0f);
			glColor4f(1, 0, 0, 1);
		}

		GLfloat pointVert[] = { p->x, p->y };
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
	QuadTree(Rectangle boundary, int n) : boundary(boundary), capacity(n), divided(false), level(0)
	{}

	void subdivide()
	{
		int x = boundary.x;
		int y = boundary.y;
		int w = boundary.w;
		int h = boundary.h;

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

		// TODO: Restore delegation
		tryDelegatePoints();
		capacity = 0;
	}

	void tryDelegatePoints()
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
				//north_west->m_points.push_back(m_points[i]);
				m_points.erase(m_points.begin() + i);
			}

			if (size != (int)m_points.size())
			{
				--i;
				size = m_points.size();
			}
		}
	}

	bool insert(Point* p)
	{
		//if(divided && m_points.size() != 0)
		//	tryDelegatePoints();

		if (!boundary.contains(*p))
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

			if (north_west->insert(p))
				return true;
			else if (north_east->insert(p))
				return true;
			else if (south_west->insert(p))
				return true;
			else if (south_east->insert(p))
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

			for (int i = 0; i < m_points.size(); i++)
			{
				if (range.contains(*m_points[i]))
				{
					result->push_back(m_points[i]);
				}
			}
			if (divided)
			{
				north_west->query(range, result);
				north_east->query(range, result);
				south_west->query(range, result);
				south_east->query(range, result);
			}
		}
		return;
	}

	int totalInserted()
	{
		int total = m_points.size();

		if (divided && total != 0)
		{
			int a; 
			a = 1;
		}

		if (divided)
		{
			total += north_west->totalInserted();
			total += north_east->totalInserted();
			total += south_west->totalInserted();
			total += south_east->totalInserted();
		}
		
		return total;
	}

	void draw()
	{
		glEnable(GL_PROGRAM_POINT_SIZE_EXT);

		if (divided)
		{
			boundary.drawBounds();
			north_west->draw();
			north_east->draw();
			south_west->draw();
			south_east->draw();
		}

		if (m_points.size())
		{
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

	

};
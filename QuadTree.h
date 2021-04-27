#pragma once
#include <vector>
#include <cassert>
#include <unordered_map>
#include <any>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
using namespace glm;
class QuadTree;
struct Point
{
	Point() : pos(0,0), data(nullptr){}
	Point(float x, float y, std::any d = nullptr) : pos(x, y), data(d){}
	glm::vec2 pos;
	QuadTree* cell;
	std::any data;
	bool initialized = false;
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


	bool insert(Point* p, float eps = 0.f)
	{
		if (p->initialized)
		{
			// adding this to only update cells vacant of points
			if (p->cell && p->cell->boundary.contains(p->pos, eps))
				return true;
			else
			{
				p->cell = nullptr;
				p->initialized = false;
			}
		
		}
		
		if (!boundary.contains(p->pos, eps))
			return false;
		
		divided = false;

		if (m_points.size() < capacity)
		{
			m_points.push_back(*p);
			p->cell = this;
			return true;
		}
		else
		{
			int x = boundary.x;
			int y = boundary.y;
			int w = boundary.w;
			int h = boundary.h;
		
	
				Rectangle ne(x + w / 2, y - h / 2, w / 2, h / 2);
				Rectangle nw(x - w / 2, y - h / 2, w / 2, h / 2);
				Rectangle se(x + w / 2, y + h / 2, w / 2, h / 2);
				Rectangle sw(x - w / 2, y + h / 2, w / 2, h / 2);

				if (ne.contains(p->pos, eps))
				{
					divided = true;
					if (!north_east)
						north_east = new QuadTree(ne, capacity);

					return north_east->insert(p);
				}
				else if (nw.contains(p->pos, eps))
				{
					divided = true;
					if (!north_west)
						north_west = new QuadTree(nw, capacity);

					return north_west->insert(p);
				}
				else if (se.contains(p->pos, eps))
				{
					divided = true;
					if (!south_east)
						south_east = new QuadTree(se, capacity);

					return south_east->insert(p);
				}
				else if (sw.contains(p->pos, eps))
				{
					divided = true;
					if (!south_west)
						south_west = new QuadTree(sw, capacity);

					return south_west->insert(p);
				}
				else
				{
					m_points.push_back(*p);
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

		if (north_west != nullptr)
		{
			north_west->clear();
			delete north_west;
			north_west = nullptr;
		}
		if (north_east != nullptr)
		{
			north_east->clear();
			delete north_east;
			north_east = nullptr;
		}
		if (south_west != nullptr)
		{
			south_west->clear();
			delete south_west;
			south_west = nullptr;

		}
		if (south_east != nullptr)
		{
			south_east->clear();
			delete south_east;
			south_east = nullptr;
		}

		divided = false;
		capacity = newCapacity;
		
		if(allBounds.size())
			allBounds.clear();
	}

	void query(Rectangle range, std::vector<Point>* result)
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
				if (north_west && north_west->boundary.intersects(range))
					north_west->query(range, result);
				if (north_east && north_east->boundary.intersects(range))
					north_east->query(range, result);
				if (south_west && south_west->boundary.intersects(range))
					south_west->query(range, result);
				if (south_east && south_east->boundary.intersects(range))
					south_east->query(range, result);
			}

			for (int i = 0; i < m_points.size(); i++)
				if (range.contains(m_points[i].pos))
 					result->push_back(m_points[i]);
		}
		return;
	}

	void GetBoundsLineSegments(std::vector<glm::vec2>& lines)
	{
		//lines = allBounds;
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

	int level = 0;

private:
	Rectangle boundary;
	int capacity;
	std::vector<Point> m_points{};

	bool divided = false;

	QuadTree* north_west;
	QuadTree* north_east;
	QuadTree* south_west;
	QuadTree* south_east;

	inline static std::vector<glm::vec2> allBounds{};

};
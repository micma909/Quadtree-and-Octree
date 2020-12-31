#pragma once
#include "QuadTree.h"


float distance(Point* p1, Point* p2)
{
	// Calculating distance 
	return sqrt(pow(p2->x - p1->x, 2) +
		pow(p2->x - p1->y, 2) * 1.0);
}


void separate(QuadTree* qTree, Point* thisBoid, float searchRadius)
{
	float desiredSeparation = 25.0;
	Point steer = Point(0, 0, 0, 0, 0);
	int count = 0;

	Rectangle searchBound(thisBoid->x, thisBoid->y, searchRadius, searchRadius);

	std::vector<Point*> boids;
	qTree->query(searchBound, &boids);

	for (int i = 0; i < boids.size(); i++)
	{
		float dist = distance(thisBoid, boids[i]);

		if ((dist > 0) && (dist < desiredSeparation))
		{
			Point diff(thisBoid->x - boids[i]->x, thisBoid->y - boids[i]->y, 0 ,0 ,0);

			float mag = sqrt((diff.x * diff.x) + (diff.y * diff.y));
			diff.x /= mag;
			diff.y /= mag;
			diff.x /= dist;
			diff.y /= dist;
			steer.x += diff.x;
			steer.y += diff.y;
			count++;
		}
	}

	if (count > 0)
	{
		steer.x /= count;
		steer.y /= count;
	}
	float mag = sqrt((steer.x * steer.x) + (steer.y * steer.y));

	if (mag > 0)
	{
		steer.x /= mag;
		steer.y /= mag;
	}

	thisBoid->vx = steer.x;
	thisBoid->vy = steer.y;

}

void flock(std::vector<Point*>* points)
{
	// for each point

	// SEPARATE
	// ALIGNMENT
	// COHESION
}
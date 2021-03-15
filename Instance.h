#pragma once

#include <glm/glm.hpp>

// switches for instance states
#define INSTANCE_DEAD		(unsigned char)0b00000001
#define INSTANCE_MOVED		(unsigned char)0b00000010

class Instance
{
public:
	// construct with parameters and default
	Instance(glm::vec3 pos, glm::vec3 size): position(pos), size(size), state(0), id(-1) {}

	// combination of switches above
	unsigned char state;
	glm::vec3 position;
	glm::vec3 size;
	int id;
};
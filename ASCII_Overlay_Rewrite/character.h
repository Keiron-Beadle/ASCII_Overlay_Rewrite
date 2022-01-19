#pragma once
#include <glm/glm/vec2.hpp>

struct character {
	glm::ivec2 size;
	glm::ivec2 bearing;
	float texcoords;
	float atlas_width;
	float atlas_height;
	unsigned int advance;
};

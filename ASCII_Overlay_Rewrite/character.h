#pragma once
#include <glm/glm/vec2.hpp>

struct character {
	unsigned int texture_id;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

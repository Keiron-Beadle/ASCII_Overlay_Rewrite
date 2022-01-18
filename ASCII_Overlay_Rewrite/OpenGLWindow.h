#pragma once
#include <iostream>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/vec2.hpp>
#include <glm/glm/ext/matrix_float4x4.hpp>
#include <glm/glm/ext/matrix_clip_space.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "character.h"
#include "constants.h"

#include <ft2build.h>
#include FT_FREETYPE_H
class OpenGLWindow
{
public:
	OpenGLWindow(int gl_major, int gl_minor, int gl_profile, const std::string& data);
	~OpenGLWindow();
private:
	GLFWwindow* window = nullptr;
	std::map<char, character> characters;
	std::string ascii_text;
	char ascii_scale[10] = { ' ','.',':',' ','.','+','=','#', '%','@' };
	unsigned int vao_id;
	unsigned int vbo_id;
	int pixel_size = constants::render_pixelsize;
	const int window_width = constants::render_width;
	const int window_height = constants::render_height;
private:
	void init_window(const int gl_major, const int gl_minor, const int gl_profile);
	void init_gl();
	void load_free_type();
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void key_callback(GLFWwindow* window, int key, int scanCode, int action, int mods);
	
};


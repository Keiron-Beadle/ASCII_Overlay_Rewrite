#pragma once
#include <iostream>
#include <GLFW/glfw3.h>
#include "constants.h"

class OpenGLWindow
{
public:
	OpenGLWindow(int gl_major, int gl_minor, int gl_profile, const std::string& data);
	~OpenGLWindow();
private:
	GLFWwindow* window = nullptr;
	std::string ascii_text;
	const int window_width = constants::render_width;
	const int window_height = constants::render_height;
private:
	void init_window(const int gl_major, const int gl_minor, const int gl_profile);
	void init_gl();
	void load_free_type();
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void key_callback(GLFWwindow* window, int key, int scanCode, int action, int mods);
	
};


#pragma once
#include <iostream>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/vec2.hpp>
#include <glm/glm/ext/matrix_float4x4.hpp>
#include <glm/glm/ext/matrix_clip_space.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "character.h"
#include "constants.h"

#include <ft2build.h>
#include <mutex>

#include FT_FREETYPE_H
class OpenGLWindow
{
public:
	OpenGLWindow(int gl_major, int gl_minor, int gl_profile, const std::string& data, std::mutex& data_mutex);
	~OpenGLWindow();
	[[nodiscard]] bool window_closing() const;
	void app_loop();
private:
	GLFWwindow* window = nullptr;
	std::map<char, character> characters;
	std::string ascii_text;
	std::unique_lock<std::mutex> render_lock;
	const char* const vertexPath = "vertexShader.vert";
	const char* const fragmentPath = "fragmentShader.frag";
	const shader* my_shader;
	unsigned int vao_id{};
	unsigned int vbo_id{};

	const float scale = 0.68f;
	int xPosition = 0;
	int yPosition = 0;
	int pixel_size = constants::render_pixelsize;
	const int window_width = constants::render_width;
	const int window_height = constants::render_height;
	static inline bool render_mode = true;
private:
	void init_window(const int gl_major, const int gl_minor, const int gl_profile);
	void init_gl();
	void render_ascii();
	void load_free_type();
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void key_callback(GLFWwindow* window, int key, int scanCode, int action, int mods);
	
};


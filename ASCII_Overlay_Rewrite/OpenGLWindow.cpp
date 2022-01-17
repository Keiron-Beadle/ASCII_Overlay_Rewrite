#include "OpenGLWindow.h"
#include "character.h"
#include <freetype/freetype.h>
#include <glad/glad.h>

OpenGLWindow::OpenGLWindow(const int gl_major, const int gl_minor, const int gl_profile, const std::string& data)
{
	init_window(gl_major, gl_minor, gl_profile);
	init_gl();
	ascii_text = data;
}

OpenGLWindow::~OpenGLWindow()
{
	glfwTerminate();
	glfwDestroyWindow(window);
}

void OpenGLWindow::init_window(const int gl_major, const int gl_minor, const int gl_profile)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, gl_profile);
	window = glfwCreateWindow(window_width, window_height, "ASCII Overlay", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Window failed to initialise." << std::endl;
		glfwTerminate();
		return;
	}
	glfwSetWindowUserPointer(window, this);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, const int width, const int height)  // NOLINT(clang-diagnostic-shadow)
		{
			OpenGLWindow::framebuffer_size_callback(window, width, height);
		});
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scanCode, int action, int mods)
		{
			OpenGLWindow::key_callback(window, key, scanCode, action, mods);
		});
}

void OpenGLWindow::init_gl()
{
	gladLoadGL();
	glViewport(0, 0, window_width, window_height);
	load_free_type();
}

void OpenGLWindow::load_free_type()
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cout << "ERROR: FREETYPE: Could not initialise" << std::endl;
		return;
	}
	FT_Face face;
	if (FT_New_Face(ft, "ibm.ttf", 0, &face)) {
		std::cout << "ERROR: FREETYPE: Failed to load font" << std::endl;
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, pixel_size);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int i = 0; i < sizeof(ASCII); i++) {
		if (FT_Load_Char(face, ASCII[i], FT_LOAD_RENDER)) {
			std::cout << "ERROR: FREETYPE: Failed to load Glyph" << std::endl;
			continue;
		}
		unsigned int tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		character character = {
			tex,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x };
		Characters.insert(std::pair<char, Character>(ASCII[i], character));
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}


void OpenGLWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
}

void OpenGLWindow::key_callback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
}

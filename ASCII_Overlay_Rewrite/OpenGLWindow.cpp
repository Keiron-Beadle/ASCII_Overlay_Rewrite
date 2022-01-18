#include "OpenGLWindow.h"

OpenGLWindow::OpenGLWindow(const int gl_major, const int gl_minor, const int gl_profile, std::string* data, std::mutex& data_mutex)
{
	init_window(gl_major, gl_minor, gl_profile);
	init_gl();
	ascii_text = data;
	render_lock = std::unique_lock (data_mutex, std::defer_lock);
}

OpenGLWindow::~OpenGLWindow()
{
	glfwTerminate();
	glfwDestroyWindow(window);
}

bool OpenGLWindow::window_closing() const
{
	return glfwWindowShouldClose(window);
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
		static_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window))->key_callback(key,scanCode,action,mods);
	});
}

void OpenGLWindow::init_gl()
{
	gladLoadGL();
	glViewport(0, 0, window_width, window_height);
	load_free_type();
	my_shader = new shader(vertexPath, fragmentPath);
	//alt_shader = new shader(vertexAltPath, fragmentAltPath);
	my_shader->use();
	//alt_shader->use();
	glGenVertexArrays(1, &vao_id);
	glGenBuffers(1, &vbo_id);
	glBindVertexArray(vao_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width), 0.0f, static_cast<float>(window_height));
	glUniformMatrix4fv(glGetUniformLocation(my_shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(glGetUniformLocation(my_shader->ID, "textColor"), 1.0f, 1.0f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void OpenGLWindow::init_render_mode(unsigned int tex)
{
	unsigned int vbo;
	glEnableVertexAttribArray(1);
	const float vertices[] = {
	1.0f,1.0f,0.0f,   1.0f, 0.0f,
	-1.0f, 1.0f,0.0f, 0.0f, 0.0f,
	1.0f,-1.0f, 0.0f, 1.0f, 1.0f,

	1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 0.0f
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glUniform1f(glGetUniformLocation(my_shader->ID, "myTex"), 0);

}

void OpenGLWindow::app_loop()
{
	//it's own capture device for when not rendering ASCII and not in need of a performance boost.
	const unsigned char* data;
	unsigned int tex = 0;
	bitblt_capture gl_capture;
	//init_render_mode(tex);
	while (!window_closing())
	{
		//double now = glfwGetTime();
		glClear(GL_COLOR_BUFFER_BIT);
		if (!render_lock.try_lock())
		{
			glfwPollEvents();
			continue;
		}
		if (render_mode)
			render_ascii();
		else
		{
			gl_capture.take_screen_shot(data);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, constants::render_width, constants::render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glDrawArrays (GL_TRIANGLES, 0,6);
		}
		render_lock.unlock();
		glfwPollEvents();
		glfwSwapBuffers(window);
		//std::cout << "Frame time: " << glfwGetTime() - now << " ms" << std::endl;
	}
	glDeleteTextures(1, &tex);
	glfwDestroyWindow(window);
	glfwTerminate();
	delete my_shader;
}

void OpenGLWindow::render_ascii()
{
	const float xStart = xPosition;
	int x = xPosition;
	int y = window_height;
	glBindVertexArray(vao_id);
	character ch;
	for (char& c : *ascii_text)
	{
		if (c == -1)
		{
			y -= constants::render_pixelsize;
			x = xStart;
			continue;
		}
		ch = characters.at(c);
		if (c == ' ')
		{
			//x += (ch.advance >> 6) ;
			x += 4;
			continue;
		}
		const float x_pos = x + ch.bearing.x ;
		const float y_pos = y - (ch.size.y - ch.bearing.y);
		const float width = ch.size.x * scale;
		const float height = ch.size.y * scale;
		float vertices[6][4] = {
			//Position                //Tex Coords
		 { x_pos,     y_pos + height,   0.0f, 0.0f },
		 { x_pos,     y_pos,       0.0f, 1.0f },
		 { x_pos + width, y_pos,       1.0f, 1.0f },

		 { x_pos,     y_pos + height,   0.0f, 0.0f },
		 { x_pos + width, y_pos,       1.0f, 1.0f },
		 { x_pos + width, y_pos + height,   1.0f, 0.0f }
		};
		glBindTexture(GL_TEXTURE_2D, ch.texture_id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), &vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += 4;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLWindow::key_callback(int key, int scanCode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_D:
		break;
	case GLFW_KEY_S:
		break;
	case GLFW_KEY_W:
		break;
	case GLFW_KEY_A:
		break;
	case GLFW_KEY_F1:
		if (action == GLFW_RELEASE)
			constants::bright_mode = !constants::bright_mode;
		break;
	case GLFW_KEY_F2:
		if (action == GLFW_RELEASE)
		{
			render_mode = !render_mode;
			if (render_mode)
			{
				my_shader->use();
			}
			else
			{
				alt_shader->use();
			}
		}
		break;
	default: ;
	}
}

void OpenGLWindow::framebuffer_size_callback(GLFWwindow* window, const int width, const int height)
{
	glViewport(0, 0, width, height);
}

void OpenGLWindow::load_free_type()
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cout << "Free Type: Could not initialise" << std::endl;
		return;
	}
	FT_Face face;
	if (FT_New_Face(ft, "ibm.ttf", 0, &face)) {
		std::cout << "Free Type: Failed to load font" << std::endl;
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, pixel_size);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (const char i : constants::ascii_scale)
	{
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			std::cout << "Free Type: Failed to load Glyph" << std::endl;
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
		character ch = {
			tex,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x };
		characters.insert(std::pair(i, ch));
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

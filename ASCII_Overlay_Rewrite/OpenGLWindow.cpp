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
	//glEnable(GL_TEXTURE_2D);
	load_free_type();
	my_shader = new shader(vertexPath, fragmentPath);
	//alt_shader = new shader(vertexAltPath, fragmentAltPath);
	my_shader->use();
	//alt_shader->use();
	glGenVertexArrays(1, &vao_id);
	glGenBuffers(1, &vbo_id);
	glBindVertexArray(vao_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id); //17976
	glBufferData(GL_ARRAY_BUFFER, 17976 * 4 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width), 0.0f, static_cast<float>(window_height));
	glUniformMatrix4fv(glGetUniformLocation(my_shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(glGetUniformLocation(my_shader->ID, "textColor"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(my_shader->ID, "text"), GL_TEXTURE0);
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
		glClear(GL_COLOR_BUFFER_BIT);
		if (!render_lock.try_lock())
		{
			glfwPollEvents();
			continue;
		}
		if (render_mode)
		{
			render_ascii();
		}
		else
		{
			gl_capture.take_screen_shot(data);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, constants::render_width, constants::render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glDrawArrays (GL_TRIANGLES, 0,6);
		}
		render_lock.unlock();
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glDeleteTextures(1, &tex);
	glfwDestroyWindow(window);
	glfwTerminate();
	delete my_shader;
}

void OpenGLWindow::render_ascii()
{
	double now = glfwGetTime();
	if (ascii_text->empty()) { return; }
	const float xStart = xPosition;
	int x = xPosition;
	int y = window_height;
	int counter = 0;
	character ch;
	for (char& c : *ascii_text)
	{
		if (c == -1)
		{
			y -= constants::render_pixelsize;
			x = xStart;
			counter++;
			continue;
		}
		ch = characters.at(c);
		if (c == ' ')
		{
			//x += (ch.advance) ;
			x += 4;
			counter++;
			continue;
		}
		const float x_pos = x + ch.bearing.x ;
		const float y_pos = y - ch.bearing.y;
		const float width = ch.size.x;
		const float height = ch.size.y;
		float vertices[6][4] = {
			//Position                //Tex Coords
		 { x_pos,     y_pos,   ch.texcoords, (height / ch.atlas_height) },
		 { x_pos,     y_pos - height,       ch.texcoords, 0.0f },
		 { x_pos + width, y_pos,       ch.texcoords + (width / ch.atlas_width), (height / ch.atlas_height)},

		 { x_pos + width, y_pos,   ch.texcoords + (width / ch.atlas_width), (height / ch.atlas_height) },
		 { x_pos, y_pos - height,       ch.texcoords, 0.0f },
		 { x_pos + width, y_pos - height,  ch.texcoords + (width / ch.atlas_width), 0.0f }
		};
		glBufferSubData(GL_ARRAY_BUFFER, counter * sizeof(vertices), sizeof(vertices), &vertices);
		counter++;
		x += 4;
	}
	glDrawArrays(GL_TRIANGLES, 0, 6 * 17976);

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
			constants::game_mode = !constants::game_mode;
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
	FT_GlyphSlot g = face->glyph;
	int w = 0;
	int h = 0;
	for (const char i : constants::ascii_scale)
	{
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			std::cout << "Free Type: Failed to load Glyph" << std::endl;
			continue;
		}
		w += g->bitmap.width;
		h = max(h, g->bitmap.rows);
	}
	const int atlas_width = w;

	//Create atlas
	unsigned int tex;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//Fill atlas
	int x = 0;
	for (const char c : constants::ascii_scale)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			continue;
		}
		character ch;
		ch = {
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<float>(x) / static_cast<float>(atlas_width),
			static_cast<float>(atlas_width),
			static_cast<float>(h),
			static_cast<unsigned int>(face->glyph->advance.x >> 6)
		};
		characters.insert(std::pair(c, ch));

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED,
			GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
		x += g->bitmap.width;

	}
	//GLubyte new_array[57 *11];
	//int sidelength;
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sidelength);
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_BYTE, new_array);
	//std::ofstream ofs("first.ppm", std::ios_base::out);
	//ofs << "P3" << std::endl << 57 << ' ' << 11 << std::endl << "255" << std::endl;
	//for (int i = 0; i < 11 * 57; i++)
	//{
	//	int value = static_cast<int>(new_array[i]);
	//	ofs << ' ' << value << ' ' << value << ' ' << value << ' ';

	//	if (i % 57  == 0 && i != 0)
	//	{
	//		ofs <<' ' << std::endl;
	//	}
	//}
	//ofs.close();
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

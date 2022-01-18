#include <iostream>
#include <thread>
#include <mutex>
#include <windows.h>

#include "OpenGLWindow.h"
#include "bitblt_capture.h"

std::string ascii_text;

void capture_thread_work(bitblt_capture* capturer, const OpenGLWindow* window)
{
	while (!window->window_closing())
	{
		const unsigned char* ascii_data;
		//capture will take screen shot,
		//then try to fill ascii_text if not locked, otherwise it'll
		//continue to next loop and get new data
		capturer->capture(ascii_data);
	}
}

int main()
{
	std::mutex ascii_mutex;
	auto window = new OpenGLWindow(3, 3, GLFW_OPENGL_CORE_PROFILE, ascii_text, ascii_mutex);
	auto capturer = new bitblt_capture(ascii_text, ascii_mutex);

	std::thread capture_thread(capture_thread_work, capturer, window);
	auto mask = static_cast<DWORD_PTR>(1) << 0;
	SetThreadAffinityMask(capture_thread.native_handle(), mask);
	SetThreadPriority(capture_thread.native_handle(), 15);
	while (!window->window_closing())
	{
		window->app_loop();
	}
	capture_thread.join();
	delete window;
	delete capturer;
}
#include <iostream>
#include <thread>
#include <mutex>
#include <windows.h>

#include "OpenGLWindow.h"
#include "bitblt_capture.h"

std::string ascii_text;
std::mutex ascii_mutex;

void capture_thread_work(bitblt_capture* capturer, const OpenGLWindow* window)
{
	std::unique_lock capture_lock (ascii_mutex, std::defer_lock);
	
	while (!window->window_closing() && OpenGLWindow::render_mode)
	{
		const unsigned char* ascii_data;
		//capture will take screen shot,
		//then try to fill ascii_text if not locked, otherwise it'll
		//continue to next loop and get new data
		capture_lock.lock();
		ascii_text = capturer->capture(ascii_data);
		capture_lock.unlock();
	}
}

int main()
{
	auto window = new OpenGLWindow(3, 3, GLFW_OPENGL_CORE_PROFILE, &ascii_text, std::ref(ascii_mutex));
	auto capturer = new bitblt_capture();

	std::thread capture_thread(capture_thread_work, capturer, window);
	auto mask = static_cast<DWORD_PTR>(1) << 0;
	SetThreadAffinityMask(capture_thread.native_handle(), mask);
	auto mask2 = static_cast<DWORD_PTR>(1) << 1;
	SetThreadAffinityMask(GetCurrentThread(), mask2);
	SetThreadPriority(capture_thread.native_handle(), 15);
	SetThreadPriority(GetCurrentThread(), 15);
	while (!window->window_closing())
	{
		window->app_loop();
	}
	capture_thread.join();
	delete window;
	delete capturer;
}
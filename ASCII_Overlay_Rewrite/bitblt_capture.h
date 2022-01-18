#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <GLFW/glfw3.h>
class bitblt_capture
{
public:
	bitblt_capture(std::string& data, std::mutex& data_mutex);
	void capture(const unsigned char*& data);
	HANDLE hDIB = nullptr;
private:
	void take_screen_shot(const unsigned char*& data);
	[[nodiscard]] static std::string parse_ascii_from_data(const unsigned char*& data);
	[[nodiscard]] static int get_grey_value(const unsigned char* const data, const int row, const int stride, const int col);
	static void adjust_brightness(const int space_counter, const int period_counter);
private:
	std::string ascii_text;
	std::unique_lock<std::mutex> capture_lock;

	RECT rc_client{};
	HWND h_wnd{};
	HDC hdc_screen{};
	HDC hdc_window{};
	HDC hdc_memDC = nullptr;
	HBITMAP hbm_screen = nullptr;
	BITMAP bmp_screen{};
	unsigned char* lp_bitmap = nullptr;
	DWORD dw_bmpSize = 0;
	BITMAPINFOHEADER bi{};
};


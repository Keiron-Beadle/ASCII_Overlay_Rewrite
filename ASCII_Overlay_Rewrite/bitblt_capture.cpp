#include "bitblt_capture.h"
#include "constants.h"

bitblt_capture::bitblt_capture()
{

	rc_client.left = constants::canvas_width; //2560
	rc_client.top = 0;
	rc_client.right = constants::canvas_width + constants::capture_width; //3584
	rc_client.bottom = constants::capture_height;
	h_wnd = GetDesktopWindow();
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = constants::capture_width;
	bi.biHeight = -constants::capture_height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	dw_bmpSize = ((constants::capture_width * bi.biBitCount + 31) / 32) * 4 * constants::capture_height;
	hDIB = GlobalAlloc(GHND, dw_bmpSize);
	lp_bitmap = static_cast<unsigned char*>(GlobalLock(hDIB)); //Lock the screen data in memory and give me a handle to it
}

std::string& bitblt_capture::capture(const unsigned char*& data) {
	take_screen_shot(data);
	ascii_text = parse_ascii_from_data(data);
	return ascii_text;
}

void bitblt_capture::take_screen_shot(const unsigned char*& data)
{
	hdc_window = GetDC(h_wnd);
	hdc_memDC = CreateCompatibleDC(hdc_window);
	if (!hdc_memDC) {
		std::cout << "Error with hdcMemDC" << std::endl;
		goto done;
	}

	hbm_screen = CreateCompatibleBitmap(hdc_window, rc_client.right - rc_client.left, rc_client.bottom - rc_client.top);
	if (!hbm_screen) {
		std::cout << "Error with hbmScreen" << std::endl;
		goto done;
	}

	SelectObject(hdc_memDC, hbm_screen);

	BitBlt(hdc_memDC, 0, 0, rc_client.right - rc_client.left,
		rc_client.bottom - rc_client.top, hdc_window, rc_client.left, rc_client.top, SRCCOPY); //Start bit block transfer of pixel data

	GetObject(hbm_screen, sizeof(BITMAP), &bmp_screen);

	if (hDIB == nullptr)
		std::cout << "hDIB IS 0" << std::endl;
	GetDIBits(hdc_window, hbm_screen, 0,
		static_cast<UINT>(bmp_screen.bmHeight),
		lp_bitmap,
		reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

done:
	if (hbm_screen == 0)
		std::cout << "hbmScreen IS 0" << std::endl;
	if (hdc_memDC == 0)
		std::cout << "hdcMemDC IS 0" << std::endl;
	DeleteObject(hbm_screen); //Cleanup objects
	DeleteObject(hdc_memDC);
	ReleaseDC(h_wnd, hdc_window);
	data = lp_bitmap; //Return handle to screenshot data
}

std::string bitblt_capture::parse_ascii_from_data(const unsigned char*& data)
{
	//For automatically adjusting brightness I keep a count of some characters, so I can adjust
	//brightness to keep a ratio of these on-screen, start at 1 so no divide by 0 error
	int space_counter = 1;
	int period_counter = 1;

	std::string output;
	//need to change 18000 to a soft value
	output.reserve(18000);
	constexpr int stride = (constants::capture_width * (32 / 8) + 3) & ~3;
	for (int row = 1; row < constants::capture_height; row++) {
		for (int col = 1; col < stride; col += 4) {

			//This part averages pixels from 4 ordinal directions and the middle to get an average brightness of that area
			constexpr int ascii_size = sizeof(constants::ascii_scale);
			const int grey_up = get_grey_value(data, row - 1, stride, col);
			const int grey_down = get_grey_value(data, row + 1, stride, col);
			const int grey_left = get_grey_value(data, row, stride, col - 4);
			const int grey_mid = get_grey_value(data, row, stride, col);
			const int grey_right = get_grey_value(data, row, stride, col + 4);
			const int avg_grey = (grey_left + grey_mid + grey_right + grey_up + grey_down) / 5;
			const char pixel_value = constants::ascii_scale[avg_grey / (ascii_size - 1)];

			if (pixel_value == '\0') { continue; }
			else if (pixel_value == ' ')
			{
				space_counter++;
			}
			else if (pixel_value == '.') { period_counter++; }

			output.push_back(pixel_value);
			col += constants::capture_pixelsize;
		}
		row += constants::capture_pixelsize;
		output.push_back(-1);
	}
	adjust_brightness(space_counter, period_counter);
	return output;
}

int bitblt_capture::get_grey_value(const unsigned char* const data, const int row, const int stride, const int col)
{
	const int row_base = row * stride;
	const int blue = data[row_base + col];
	const int green = data[row_base + col + 1];
	const int red = data[row_base + col + 2];
	const int grey = (red * 0.299) / 3 + (green * 0.587) / 3 + (blue * 0.114) / 3;
	return constants::brightness * grey;
}

void bitblt_capture::adjust_brightness(const int space_counter, const int period_counter)
{
	const float space_percent = static_cast<float>(space_counter) / 18000;
	const float period_percent = static_cast<float>(period_counter) / 18000;
	const float space_threshold_upper = constants::game_mode ? constants::upper_space_limit : 0.6f;
	const float space_threshold_lower = constants::game_mode ? constants::lower_space_limit : 0.05f;
	const float period_threshold = constants::game_mode ? 0.4f : 0.9f;

	if (space_percent > space_threshold_upper)
	{
		if (constants::brightness < 0.99f)
			constants::brightness += 0.01f;
	}
	else if (space_percent < space_threshold_lower)
	{
		if (constants::brightness > 0.01f)
			constants::brightness -= 0.01f;
	}
	else if (period_percent > period_threshold)
	{
		if (constants::brightness < 0.99f)
			constants::brightness += 0.01f;
	}
}

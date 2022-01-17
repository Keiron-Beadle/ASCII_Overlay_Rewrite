#pragma once
class constants {
public:
	static constexpr int capture_width = 1280;
	static constexpr int capture_height = 720;
	static constexpr int render_width = 2560;
	static constexpr int render_height = 1440;
	static constexpr int canvas_width = 2560;

	static constexpr int capture_pixelsize = 12;
	static constexpr int render_pixelsize = capture_pixelsize * 4;

	// 0 = MAKE SCENE DARKER
	// 1 = MAKE SCENE BRIGHTER
	float brightness;

	constants() {
		brightness = 0.5f;
	}
};
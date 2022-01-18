#pragma once
class constants {
public:
	static constexpr int capture_width = 1280;
	static constexpr int capture_height = 720;
	static constexpr int render_width = 1280;
	static constexpr int render_height = 720;
	static constexpr int canvas_width = 2560;

	static constexpr int capture_pixelsize = 12;
	static constexpr int render_pixelsize = capture_pixelsize;

	// 0 = MAKE SCENE DARKER
	// 1 = MAKE SCENE BRIGHTER
	static inline float brightness;
	static inline bool bright_mode;
	static constexpr float upper_space_limit = 0.3f;
	static constexpr float lower_space_limit = 0.1f;

	static constexpr char ascii_scale[10] = { ' ','.',':',' ','.','+','=','#', '%','@' };

	constants() {
		brightness = 0.5f;
	}
};
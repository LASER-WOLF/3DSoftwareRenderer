#include "display.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static uint32_t* color_buffer = NULL;
static float* z_buffer = NULL;

static SDL_Texture* color_buffer_texture = NULL;
static int window_width = 640;  //320
static int window_height = 480; //200

static int render_method = 0;
static int cull_method = 0;

int get_window_width(void) {
	return window_width;
}

int get_window_height(void) {
	return window_height;
}

bool init_window(void) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL.\n");
		return false;
	}
	
	// Use SDL to query what the fullscreen max width and height
	SDL_DisplayMode display_mode;
	SDL_GetCurrentDisplayMode(0, &display_mode);
	int fullscreen_width = display_mode.w;
	int fullscreen_height = display_mode.h;

	float aspect_ratio = (float)fullscreen_width / (float)fullscreen_height;
	window_height = window_width / aspect_ratio;

	// Create a SDL Window
	window = SDL_CreateWindow(
		NULL, 
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		fullscreen_width,
		fullscreen_height,
		SDL_WINDOW_BORDERLESS
	);
	if (!window) {
		fprintf(stderr, "Error creating SDL window.\n");
		return false;
	}
	
	// Create a SDL renderer
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Error creating SDL renderer.\n");
		return false;
	}

	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	// Allocate the required bytes in memory to hold the color buffer and the z-buffer
	color_buffer = (uint32_t*)malloc(sizeof(uint32_t) * window_width * window_height);
	z_buffer = (float*)malloc(sizeof(float) * window_width * window_height);

	// Creating a SDL texture that is used to display the color buffer
	color_buffer_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);

	return true;
}

void set_render_method(int method) {
	render_method = method;
}

void set_cull_method(int method) {
	cull_method = method;
}

bool should_cull_backface(void) {
	return cull_method == CULL_BACKFACE;
}

bool should_render_wire(void) {
	return (
		render_method == RENDER_WIRE || 
		render_method == RENDER_WIRE_VERTEX || 
		render_method == RENDER_FILL_TRIANGLE_WIRE || 
		render_method == RENDER_TEXTURED_WIRE
	);
}

bool should_render_wire_vertex(void) {
	return (
		render_method == RENDER_WIRE_VERTEX
	);
}

bool should_render_filled_triangle(void) {
	return (
		render_method == RENDER_FILL_TRIANGLE || 
		render_method ==RENDER_FILL_TRIANGLE_WIRE
	);
}

bool should_render_textured_triangle(void) {
	return (
		render_method == RENDER_TEXTURED || 
		render_method == RENDER_TEXTURED_WIRE
	);
}

void draw_grid(void) {
	for (int y = 0; y < window_height; y += 10) {
		for (int x = 0; x < window_width; x += 10) {
			color_buffer[(window_width * y) + x] = 0xFF444444;
		}
	}
}

void draw_pixel(int x, int y, uint32_t color) {
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return;
	}
	color_buffer[(window_width * y) + x] = color;
}

// DDA line drawing algorithm
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
	int delta_x = (x1 - x0);
	int delta_y = (y1 - y0);

	int longest_side_length = abs(delta_x) >= abs(delta_y) ? abs(delta_x) : abs(delta_y);

	float x_inc = delta_x / (float)longest_side_length;
	float y_inc = delta_y / (float)longest_side_length;

	float current_x = x0;
	float current_y = y0;
	for (int i = 0; i <= longest_side_length; i++) {
		draw_pixel(round(current_x), round(current_y), color);
		current_x += x_inc;
		current_y += y_inc;
	}
}

void draw_rect(int start_x, int start_y, int width, int height, uint32_t color) {
	for (int y = start_y; y < (start_y + height); y++) {
		for (int x = start_x; x < (start_x + width); x++) {
			draw_pixel(x, y, color);
		}
	}
}

void render_color_buffer(void) {
	SDL_UpdateTexture(
		color_buffer_texture,
		NULL,
		color_buffer,
		(int) (window_width * sizeof(uint32_t))
		);
	SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void clear_color_buffer(uint32_t color) {
	for (int i = 0; i < window_height * window_width; i++)
		color_buffer[i] = color;
}

void clear_z_buffer(void) {
	for (int i = 0; i < window_height * window_width; i++)
		z_buffer[i] = 1.0;
}

float get_zbuffer_at(int x, int y) {
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return 1.0;
	}
	return z_buffer[(window_width * y) + x];
}

void update_zbuffer_at(int x, int y, float value) {
	if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
		return;
	}
	z_buffer[(window_width * y) + x] = value;
}

void destroy_window(void) {
	free(color_buffer);
	free(z_buffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

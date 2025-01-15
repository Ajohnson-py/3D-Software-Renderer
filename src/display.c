#include "display.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static uint32_t* color_buffer = NULL;
static float* z_buffer = NULL;
static SDL_Texture* color_buffer_texture = NULL;

static int window_width = 480;
static int window_height = 360;

static int render_method = 0;
static int cull_method = 0;

bool initalize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initalizing SDL.\n");
        return false;
    } 
    
    // Use SDL to find fullscreen width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    int fullscreen_width = display_mode.w;
    int fullscreen_height = display_mode.h;

    // Change resolution to get pixelated effect
    window_width = fullscreen_width / 1;
    window_height = fullscreen_height / 1;

    // Create a SDL window
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

    // Create SDL renderer
    renderer = SDL_CreateRenderer(window, -1, 0);

    if (!renderer) {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }

    // Put window in fullscreen mode
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    // Hide mouse and lock it to center of screen
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Allocate required memory to hold color buffer and z buffer
    color_buffer = (uint32_t*) malloc(sizeof(uint32_t) * window_width * window_height);
    z_buffer = (float*) malloc(sizeof(float) * window_width * window_height);

    // Create SDL texture that will display color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height
    );

    return true;
}

int get_window_width(void) {
    return window_width;
}

int get_window_height(void) {
    return window_height;
}

void set_render_method(int method) {
    render_method = method;
}

void set_cull_method(int method) {
    cull_method = method;
}

bool is_cull_backface(void) {
    return cull_method == CULL_BACKFACE;
}

bool should_render_filled_triangles(void) {
    return (
        render_method == RENDER_FILL_TRIANGLE || 
        render_method == RENDER_FILL_TRIANGLE_WIRE
    );
}

bool should_render_textured_triangles(void) {
    return (
        render_method == RENDER_TEXTURED || 
        render_method == RENDER_TEXTURED_WIRE
    );
}

bool should_render_wireframe(void) {
    return (
        render_method == RENDER_WIRE || 
        render_method == RENDER_FILL_TRIANGLE_WIRE || 
        render_method == RENDER_WIRE_VERTEX || 
        render_method == RENDER_TEXTURED_WIRE
        );
}

bool should_render_vertices(void) {
    return render_method == RENDER_WIRE_VERTEX;
}

void draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
        return;
    }

    color_buffer[(y * window_width) + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    // Handle horizontal line case
    if (y0 == y1) {
        if (x0 < x1) {
            for (int x = x0; x <= x1; x++) {
                draw_pixel(x, y0, color);
            }
        } 
        else {
            for (int x = x1; x <= x0; x++) {
                draw_pixel(x, y0, color);
            }
        }

        return;
    }

    // Handle all other cases
    int delta_x = (x1 - x0);
    int delta_y = (y1 - y0);

    int side_length = (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);

    float x_inc = delta_x / (float)side_length;
    float y_inc = delta_y / (float)side_length;

    float current_x = x0;
    float current_y = y0;

    for (int i = 0; i <= side_length; i++) {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

void draw_grid(void) {
    for (int y = 0; y < window_height; y += 10) {
        for (int x = 0; x < window_width; x += 10) {
            color_buffer[(window_width * y) + x] = 0xFF444444;
        }
    }
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int y_pos = y; y_pos <= y + height; y_pos++) {
        for (int x_pos = x; x_pos <= x + width; x_pos++) {
            draw_pixel(x_pos, y_pos, color);
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

    SDL_RenderCopy(
        renderer,
        color_buffer_texture,
        NULL,
        NULL
    );

    SDL_RenderPresent(renderer);
}

void clear_color_buffer(uint32_t color) {
    for (int i = 0; i < window_width * window_height; i++) {
        color_buffer[i] = color;
    }
}

void clear_z_buffer(void) {
    for (int i = 0; i < window_width * window_height; i++) {
        z_buffer[i] = 1.0;
    }
}

float get_z_buffer_at(int x, int y) {
    if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
        return 1.0;
    }

    return z_buffer[(y * window_width) + x];
}

void update_z_buffer_at(int x, int y, float value) {
    if (x < 0 || x >= window_width || y < 0 || y >= window_height) {
        return;
    }

    z_buffer[(y * window_width) + x] = value;
}

void destroy_window(void) {
    free(color_buffer);
    free(z_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(color_buffer_texture);
    SDL_Quit();
}
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <stdint.h>
#include "vector.h"
#include "texture.h"
#include "upng.h"
#include "display.h"
#include "swap.h"

typedef struct {
    int a;
    int b;
    int c;
    tex2_t a_uv;
    tex2_t b_uv;
    tex2_t c_uv;
    uint32_t color;
} face_t;

typedef struct {
    vec4_t points[3];
    tex2_t tex_coords[3];
    uint32_t color;
    upng_t* texture;
} triangle_t;

vec3_t get_triangle_normal(vec4_t vertices[3]);

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void draw_filled_triangle(triangle_t triangle, uint32_t color);
void draw_texel(int x, int y, upng_t* texture, vec4_t point_a, vec4_t point_b, vec4_t point_c, tex2_t a_uv, tex2_t b_uv, tex2_t c_uv);
void draw_textured_triangle(triangle_t triangle);

#endif
#include "triangle.h"

vec3_t get_triangle_normal(vec4_t vertices[3]) {
    vec3_t vector_a = vec3_from_vec4(vertices[0]);
    vec3_t vector_b = vec3_from_vec4(vertices[1]);
    vec3_t vector_c = vec3_from_vec4(vertices[2]);

    vec3_t vector_ab = vec3_sub(vector_b, vector_a);
    vec3_t vector_ac = vec3_sub(vector_c, vector_a);
    vec3_normalize(&vector_ab);
    vec3_normalize(&vector_ac);

    vec3_t normal = vec3_cross(vector_ab, vector_ac);
    vec3_normalize(&normal);

    return normal;
}

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
    vec2_t ac = vec2_sub(c, a);
    vec2_t ab = vec2_sub(b, a);
    vec2_t ap = vec2_sub(p, a);
    vec2_t pc = vec2_sub(c, p);
    vec2_t pb = vec2_sub(b, p);

    // Find the area of the parallelogram corresponding to triangle ABC
    float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x);

    // Find the value of alpha
    float alpha = (pc.x * pb.y - pc.y * pb.x ) / area_parallelogram_abc;

    // Find the value of beta
    float beta = (ac.x * ap.y - ac.y * ap.x ) / area_parallelogram_abc;

    // Find the value of gamma
    float gamma = 1 - alpha - beta;

    vec3_t weights = {alpha, beta, gamma};
    return weights;
}

void draw_triangle_pixel(int x, int y, uint32_t color, vec4_t point_a, vec4_t point_b, vec4_t point_c) {
    // Create variables to find interpolation
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);

    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // Interpolate the value of 1/w
    float interpolated_inv_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;

    // Adjust 1/w so pixels closer to the camera have a smaller value
    interpolated_inv_w = 1 - interpolated_inv_w;

    // Draw pixel at (x, y) with a solid color if the pixel's z-value is less than the value previously stored in the z-buffer
    if (interpolated_inv_w < get_z_buffer_at(x, y)) {
        draw_pixel(x, y, color);
        update_z_buffer_at(x, y, interpolated_inv_w);
    }
}

void draw_texel(int x, int y, upng_t* texture, vec4_t point_a, vec4_t point_b, vec4_t point_c, tex2_t a_uv, tex2_t b_uv, tex2_t c_uv) {
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);

    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // Variables to store the interpolated values of U, V, and the inverse of w
    float interpolated_u;
    float interpolated_v;
    float interpolated_inv_w;

    // Store inverse w-values to save time
    float point_a_inv_w = 1 / point_a.w;
    float point_b_inv_w = 1 / point_b.w;
    float point_c_inv_w = 1 / point_c.w;

    // Perform the interpolation of all U/w and V/w values using barycentric weights and a factor of 1/w
    interpolated_u = (a_uv.u * point_a_inv_w) * alpha + (b_uv.u * point_b_inv_w) * beta + (c_uv.u * point_c_inv_w) * gamma;
    interpolated_v = (a_uv.v * point_a_inv_w) * alpha + (b_uv.v * point_b_inv_w) * beta + (c_uv.v * point_c_inv_w) * gamma;

    // Interpolate the value of 1/w
    interpolated_inv_w = point_a_inv_w * alpha + point_b_inv_w * beta + point_c_inv_w * gamma;

    // Divide both interpolated values by 1/w 
    interpolated_u /= interpolated_inv_w;
    interpolated_v /= interpolated_inv_w;

    // Get texture width and height
    int texture_width = upng_get_width(texture);
    int texture_height = upng_get_height(texture);

    // Map the UV coordinate to the full texture width and height
    int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width;
    int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;
    
    // Adjust 1/w so pixels closer to the camera have a smaller value
    interpolated_inv_w = 1.0 - interpolated_inv_w;

    // Draw pixel at (x, y) with color from texture map only if the pixel's z-value is less than the value previously stored in the z-buffer
    if (interpolated_inv_w < get_z_buffer_at(x, y)) {
        // Get color buffer from texture
        uint32_t* texture_buffer = (uint32_t*)upng_get_buffer(texture);

        draw_pixel(x, y, texture_buffer[tex_y * texture_width + tex_x]);
    
        // Update z-buffer value with 1/w for the current pixel
        update_z_buffer_at(x, y, interpolated_inv_w);
    }
}

void draw_textured_triangle(triangle_t triangle) {
    // Initialize texture
    upng_t* texture = triangle.texture;

    // Info for the first point
    int x0 = triangle.points[0].x;
    int y0 = triangle.points[0].y;
    float z0 = triangle.points[0].z; 
    float w0 = triangle.points[0].w; 
    float u0 = triangle.tex_coords[0].u;
    float v0 = triangle.tex_coords[0].v;

    // Info for the second point
    int x1 = triangle.points[1].x;
    int y1 = triangle.points[1].y;
    float z1 = triangle.points[1].z; 
    float w1 = triangle.points[1].w; 
    float u1 = triangle.tex_coords[1].u;
    float v1 = triangle.tex_coords[1].v;

    // Info for third second point
    int x2 = triangle.points[2].x;
    int y2 = triangle.points[2].y;
    float z2 = triangle.points[2].z; 
    float w2 = triangle.points[2].w; 
    float u2 = triangle.tex_coords[2].u;
    float v2 = triangle.tex_coords[2].v;

    // Sort vertices by ascending y-coordinates
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }

    // Flip V component to account for inverted UV coordinates
    v0 = 1.0 - v0;
    v1 = 1.0 - v1;
    v2 = 1.0 - v2;

    // Create vector points and texture coordinates for the vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};
    tex2_t a_uv = {u0, v0};
    tex2_t b_uv = {u1, v1};
    tex2_t c_uv = {u2, v2};

    // Render the flat-top triangle
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            // Swap if x_start is to the right of x_end
            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x <= x_end; x++) {
                // Draw pixel with the color from the texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv, c_uv);
            }
        }
    }

    // Render the flat-bottom triangle
    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            // Swap if x_start is to the right of x_end
            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x <= x_end; x++) {
                // Draw pixel with the color from the texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv, c_uv);
            }
        }
    }
}

void draw_filled_triangle(triangle_t triangle, uint32_t color) {
    // Info for the first point
    int x0 = triangle.points[0].x;
    int y0 = triangle.points[0].y;
    float z0 = triangle.points[0].z;
    float w0 = triangle.points[0].w;

    // Info for the second point
    int x1 = triangle.points[1].x;
    int y1 = triangle.points[1].y;
    float z1 = triangle.points[1].z;
    float w1 = triangle.points[1].w;

    // Info for third second point
    int x2 = triangle.points[2].x;
    int y2 = triangle.points[2].y;
    float z2 = triangle.points[2].z;
    float w2 = triangle.points[2].w;

    // Sort vertices by ascending y-coordinates
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
    }

    // Create vector points for the vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};

    // Render the flat-top triangle
    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            // Swap if x_start is to the right of x_end
            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x <= x_end; x++) {
                // Draw pixel with color
                draw_triangle_pixel(x, y, color, point_a, point_b, point_c);
            }
        }
    }

    // Render the flat-bottom triangle
    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;

            // Swap if x_start is to the right of x_end
            if (x_end < x_start) int_swap(&x_start, &x_end);

            for (int x = x_start; x <= x_end; x++) {
                // Draw pixel with color
                draw_triangle_pixel(x, y, color, point_a, point_b, point_c);
            }
        }
    }
}

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "array.h"
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "triangle.h"
#include "matrix.h"
#include "light.h"
#include "texture.h"
#include "upng.h"
#include "camera.h"
#include "clipping.h"

triangle_t* triangles_to_render = NULL;
int num_triangles_to_render = 0;

bool is_running = false;
uint64_t previous_frame_time = 0;
float delta_time = 0;

mat4_t world_matrix;
mat4_t proj_matrix;
mat4_t view_matrix;

void setup(void) {
    // Initialize render mode and triangle culling method
    set_render_method(RENDER_TEXTURED);
    set_cull_method(CULL_BACKFACE);

    // Initialize scene light direction
    init_light(vec3_new(0, 0, 1));

    // Initialize scene camera
    init_camera(vec3_new(0, 0, 0), vec3_new(0, 0, 1));

    // Initialize perspective projection matrix
    float aspect_x = (float)get_window_width() / get_window_height();
    float aspect_y = (float)get_window_height() / get_window_width();
    float fov_y = M_PI / 3;
    float fov_x = atan(tan(fov_y / 2) * aspect_x) * 2.0;
    float z_near = 0.1;
    float z_far = 100.0;
    proj_matrix = mat4_make_perspective(fov_y, aspect_y, z_near, z_far);

    // Initialize frustum planes
    init_frustum_planes(fov_x, fov_y, z_near, z_far);

    // Load mesh data (OBJ and PNG texture) and apply initialized transformations
    load_mesh("./assets/crab.obj", "./assets/crab.png", vec3_new(1, 1, 1), vec3_new(-3, 0, 10), vec3_new(0, 0, 0));
    load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_new(1, 1, 1), vec3_new(3, 0, 10), vec3_new(0, 0, 0));

    // Find maximum number of triagles in the meshes and ininialize the size of triangles_to_render
    int max_size = 0;
    for (int i = 0; i < get_num_meshes(); i++) {
        mesh_t* mesh = get_mesh(i);
        int num_triangles = array_length(mesh->faces);

        if (num_triangles > max_size) {
            max_size = num_triangles;
        }
    }
    triangles_to_render = (triangle_t*)malloc(sizeof(triangle_t) * max_size);
}

void process_input(void) {
    SDL_Event event;
    int mouse_dx = 0, mouse_dy = 0; 

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                is_running = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    is_running = false;
                    break;
                }
                // Keys to change render mode
                if (event.key.keysym.sym == SDLK_1) set_render_method(RENDER_WIRE_VERTEX);
                if (event.key.keysym.sym == SDLK_2) set_render_method(RENDER_WIRE);
                if (event.key.keysym.sym == SDLK_3) set_render_method(RENDER_FILL_TRIANGLE);
                if (event.key.keysym.sym == SDLK_4) set_render_method(RENDER_FILL_TRIANGLE_WIRE);
                if (event.key.keysym.sym == SDLK_5) set_render_method(RENDER_TEXTURED);
                if (event.key.keysym.sym == SDLK_6) set_render_method(RENDER_TEXTURED_WIRE);
                if (event.key.keysym.sym == SDLK_c) set_cull_method(CULL_BACKFACE);
                if (event.key.keysym.sym == SDLK_x) set_cull_method(CULL_NONE);
                break;

            case SDL_MOUSEMOTION:
                mouse_dx = event.motion.xrel;
                mouse_dy = event.motion.yrel;
                update_camera_yaw(mouse_dx * 0.01f); 
                update_camera_pitch(mouse_dy * 0.01f); 
                break;

            default:
                break;
        }
    }

    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    vec3_t forward = get_camera_direction();
    vec3_t right = vec3_cross(forward, vec3_new(0, 1, 0)); 

    // Keys for movement
    if (keystate[SDL_SCANCODE_W]) {
        update_camera_forward_velocity(vec3_mul(forward, 5.0 * delta_time));
        update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keystate[SDL_SCANCODE_S]) {
        update_camera_forward_velocity(vec3_mul(forward, -5.0 * delta_time));
        update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keystate[SDL_SCANCODE_A]) {
        update_camera_sideways_velocity(vec3_mul(right, 5.0 * delta_time));
        update_camera_position(vec3_add(get_camera_position(), get_camera_sideways_velocity()));
    }
    if (keystate[SDL_SCANCODE_D]) {
        update_camera_sideways_velocity(vec3_mul(right, -5.0 * delta_time));
        update_camera_position(vec3_add(get_camera_position(), get_camera_sideways_velocity()));
    }
    if (keystate[SDL_SCANCODE_SPACE]) {
        update_camera_position(vec3_add(get_camera_position(), vec3_new(0, 5.0 * delta_time, 0))); 
    }
    if (keystate[SDL_SCANCODE_LCTRL]) {
        update_camera_position(vec3_add(get_camera_position(), vec3_new(0, -5.0 * delta_time, 0)));
    }
}

void process_graphics_pipeline_stages(mesh_t* mesh) {
    // Create a view matrix
    vec3_t target = get_camera_look_at_target(); //{0, 0, 1};
    vec3_t up_direction = vec3_new(0, 1, 0);
    view_matrix = mat4_look_at(get_camera_position(), target, up_direction);

    // Create scale, translation, and rotation matrices to scale the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
    mat4_t translation_matrix = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);
    mat4_t rotation_x_matrix = mat4_make_rotation_x(mesh->rotation.x);
    mat4_t rotation_y_matrix = mat4_make_rotation_y(mesh->rotation.y);
    mat4_t rotation_z_matrix = mat4_make_rotation_z(mesh->rotation.z);

    // Crate a world matrix based on scale, rotation, and translation
    world_matrix = mat4_identity();
    world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_x_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_y_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_z_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

    // Loop through all triangle faces of mesh
    int num_faces = array_length(mesh->faces);
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh->faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh->vertices[mesh_face.a];
        face_vertices[1] = mesh->vertices[mesh_face.b];
        face_vertices[2] = mesh->vertices[mesh_face.c];

        // Loop through all 3 vertices of current face and apply transformations
        vec4_t transformed_vertices[3];
        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Apply world matrix to vertex so we convert to world space
            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            // Apply view matrix to vertex so we convert to camera space
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            // Save transformed vertex in array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        // Calculate triangle face normal
        vec3_t face_normal = get_triangle_normal(transformed_vertices);

        // Perform backface culling if needed
        if (is_cull_backface()) {
            vec3_t camera_ray = vec3_sub(vec3_new(0, 0, 0), vec3_from_vec4(transformed_vertices[0]));

            float dot_normal_camera = vec3_dot(face_normal, camera_ray);

            // Do not render triangle if it's not visible by camera
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        // Create a polygon from triangle and perform clipping
        polygon_t polygon = polygon_from_triangle(
            vec3_from_vec4(transformed_vertices[0]),
            vec3_from_vec4(transformed_vertices[1]),
            vec3_from_vec4(transformed_vertices[2]),
            mesh_face.a_uv,
            mesh_face.b_uv,
            mesh_face.c_uv
        );

        clip_polygon(&polygon);

        // Split polygon back into triangles
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;

        triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

        // Loop through all new triangles created after clipping 
        for (int t = 0; t < num_triangles_after_clipping; t++) {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];
            vec4_t projected_points[3];

            // Loop through all 3 vertices of current face and project them
            for (int j = 0; j < 3; j++) {
                // Project current vertex
                projected_points[j] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[j]);

                // Scale into view
                projected_points[j].x *= get_window_width() / 2.0;
                projected_points[j].y *= get_window_height() / 2.0;

                // Invert y values to account for flipped screen y-coordinate system
                projected_points[j].y *= -1;

                // Translate to middle of screen
                projected_points[j].x += (get_window_width() / 2.0);
                projected_points[j].y += (get_window_height() / 2.0);
            }

            // Perform flat shading on triangle face to find its new color based on lighting
            float light_intensity_factor = -vec3_dot(face_normal, get_light_direction());
            uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

            triangle_t triangle_to_render = {
                .points = {
                    {projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                    {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                    {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w}
                },
                .tex_coords = {
                    {triangle_after_clipping.tex_coords[0].u, triangle_after_clipping.tex_coords[0].v},
                    {triangle_after_clipping.tex_coords[1].u, triangle_after_clipping.tex_coords[1].v},
                    {triangle_after_clipping.tex_coords[2].u, triangle_after_clipping.tex_coords[2].v}
                },
                .color = triangle_color,
                .texture = mesh->texture
            };

            // Save projected triangle to array of triangles to render
            triangles_to_render[num_triangles_to_render] = triangle_to_render;
            num_triangles_to_render++;
        }
    }
}

void update(void) {
    // Implement fixed time steps
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    if (time_to_wait > 0) {
        SDL_Delay(time_to_wait);
    }

    // Get delta time factor (in seconds)
    delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;
    
    previous_frame_time = SDL_GetTicks();

    // Initialize counter of triangles to render for the current frame
    num_triangles_to_render = 0;

    // Loop through all meshes in scene and disply them on screen
    for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++) {
        mesh_t* mesh = get_mesh(mesh_index);

        // Update rotation, scale, and translation
        mesh->rotation.x += 0.0 * delta_time;
        mesh->rotation.y += 0.0 * delta_time;
        mesh->rotation.z += 0.0 * delta_time;
        mesh->translation.z = 5;             // move mesh away from camera

        process_graphics_pipeline_stages(mesh);
    }
}

void render(void) {
    // Clear all arrays to prepare for rendering
    clear_color_buffer(0xFF000000);
    clear_z_buffer();

    // Draw background grid
    draw_grid();

    // Loop all projected triangles and render them  
    for (int i = 0; i < num_triangles_to_render; i++) {
        triangle_t triangle = triangles_to_render[i];
        
        // Draw filled triangles
        if (should_render_filled_triangles()) {
            draw_filled_triangle(triangle, triangle.color);
        }

        //Draw textured triangle
        if (should_render_textured_triangles()) {
            draw_textured_triangle(triangle);
        }

        // Draw triangle wireframe
        if (should_render_wireframe()) {
            draw_triangle(
                triangle.points[0].x, triangle.points[0].y, // vertex A
                triangle.points[1].x, triangle.points[1].y, // vertex B
                triangle.points[2].x, triangle.points[2].y, // vertex C
                0xFFFFFFFF                                  // color
            );
        } 

        // Draw vertices
        if (should_render_vertices()) {
            for (int i = 0; i < sizeof(triangle.points); i++) 
                draw_rect(triangle.points[i].x - 3, triangle.points[i].y - 3, 6, 6, 0xFF0000FF);
        }
    }

    render_color_buffer();
}

void free_resources(void) {
    destroy_window();
    free_meshes();
}

int main(void) {
    is_running = initalize_window();

    setup();

    while (is_running) {
        process_input();
        update();
        render();
    }
    
    destroy_window();
    free_resources();

    return 0;
}

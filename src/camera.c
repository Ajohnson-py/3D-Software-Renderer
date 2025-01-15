#include "camera.h"

static camera_t camera;

void init_camera(vec3_t position, vec3_t direction) {
    camera.position = position;
    camera.direction = direction;
    camera.forward_velocity = vec3_new(0, 0, 0);
    camera.sideways_velocity = vec3_new(0, 0, 0);
    camera.pitch = 0.0;
    camera.yaw = 0.0;
}

vec3_t get_camera_position(void) {
    return camera.position;
}

vec3_t get_camera_direction(void) {
    return camera.direction;
}

vec3_t get_camera_forward_velocity(void) {
    return camera.forward_velocity;
}

vec3_t get_camera_sideways_velocity(void) {
    return camera.sideways_velocity;
}

float get_camera_yaw(void) {
    return camera.yaw;
}

float get_camera_pitch(void) {
    return camera.pitch;
}

void update_camera_position(vec3_t position) {
    camera.position = position;
}

void update_camera_direction(vec3_t direction) {
    camera.direction = direction;
}

void update_camera_forward_velocity(vec3_t forward_velocity) {
    camera.forward_velocity = forward_velocity;
}

void update_camera_sideways_velocity(vec3_t sideways_velocity) {
    camera.sideways_velocity = sideways_velocity;
}

void update_camera_yaw(float yaw) {
    camera.yaw += yaw;
}

void update_camera_pitch(float pitch) {
    camera.pitch += pitch;
}

vec3_t get_camera_look_at_target(void) {
    // Initialize camera so it looks into positive z axis
    vec3_t target = vec3_new(0, 0, 1);

    mat4_t pitch_rotation_matrix = mat4_make_rotation_x(camera.pitch);
    mat4_t yaw_rotation_matrix = mat4_make_rotation_y(camera.yaw);

    // Create rotation matrix based on pitch and yaw
    mat4_t rotation_matrix = mat4_identity();
    rotation_matrix = mat4_mul_mat4(pitch_rotation_matrix, rotation_matrix);
    rotation_matrix = mat4_mul_mat4(yaw_rotation_matrix, rotation_matrix);

    // Update camera direction to account for rotation
    vec4_t camera_direction = mat4_mul_vec4(rotation_matrix, vec4_from_vec3(target));
    camera.direction = vec3_from_vec4(camera_direction);

    // Offset target based on camera position and return result
    target = vec3_add(camera.position, camera.direction);

    return target;
}
#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"
#include "matrix.h"

typedef struct {
    vec3_t position;
    vec3_t direction;
    vec3_t forward_velocity;
    vec3_t sideways_velocity;
    float pitch;
    float yaw;
} camera_t;

void init_camera(vec3_t position, vec3_t direction);

vec3_t get_camera_position(void);
vec3_t get_camera_direction(void);
vec3_t get_camera_forward_velocity(void);
vec3_t get_camera_sideways_velocity(void);
float get_camera_yaw(void);
float get_camera_pitch(void);

void update_camera_position(vec3_t delta_position);
void update_camera_direction(vec3_t delta_position);
void update_camera_forward_velocity(vec3_t forward_velocity);
void update_camera_sideways_velocity(vec3_t sideways_velocity);
void update_camera_yaw(float yaw);
void update_camera_pitch(float pitch);

vec3_t get_camera_look_at_target(void);

#endif
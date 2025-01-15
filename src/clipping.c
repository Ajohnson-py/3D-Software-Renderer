#include "clipping.h"
#include <math.h>
#include "vector.h"

#define NUM_PLANES 6
plane_t frustum_planes[NUM_PLANES];

void init_frustum_planes(float fov_x, float fov_y, float z_near, float z_far) {
	float cos_half_fov_x = cos(fov_x / 2);
	float sin_half_fov_x = sin(fov_x / 2);
    float cos_half_fov_y = cos(fov_y / 2);
	float sin_half_fov_y = sin(fov_y / 2);

	frustum_planes[LEFT_FRUSTUM_PLANE].point = vec3_new(0, 0, 0);
	frustum_planes[LEFT_FRUSTUM_PLANE].normal.x = cos_half_fov_x;
	frustum_planes[LEFT_FRUSTUM_PLANE].normal.y = 0;
	frustum_planes[LEFT_FRUSTUM_PLANE].normal.z = sin_half_fov_x;

	frustum_planes[RIGHT_FRUSTUM_PLANE].point = vec3_new(0, 0, 0);
	frustum_planes[RIGHT_FRUSTUM_PLANE].normal.x = -cos_half_fov_x;
	frustum_planes[RIGHT_FRUSTUM_PLANE].normal.y = 0;
	frustum_planes[RIGHT_FRUSTUM_PLANE].normal.z = sin_half_fov_x;

	frustum_planes[TOP_FRUSTUM_PLANE].point = vec3_new(0, 0, 0);
	frustum_planes[TOP_FRUSTUM_PLANE].normal.x = 0;
	frustum_planes[TOP_FRUSTUM_PLANE].normal.y = -cos_half_fov_y;
	frustum_planes[TOP_FRUSTUM_PLANE].normal.z = sin_half_fov_y;

	frustum_planes[BOTTOM_FRUSTUM_PLANE].point = vec3_new(0, 0, 0);
	frustum_planes[BOTTOM_FRUSTUM_PLANE].normal.x = 0;
	frustum_planes[BOTTOM_FRUSTUM_PLANE].normal.y = cos_half_fov_y;
	frustum_planes[BOTTOM_FRUSTUM_PLANE].normal.z = sin_half_fov_y;

	frustum_planes[NEAR_FRUSTUM_PLANE].point = vec3_new(0, 0, z_near);
	frustum_planes[NEAR_FRUSTUM_PLANE].normal.x = 0;
	frustum_planes[NEAR_FRUSTUM_PLANE].normal.y = 0;
	frustum_planes[NEAR_FRUSTUM_PLANE].normal.z = 1;

	frustum_planes[FAR_FRUSTUM_PLANE].point = vec3_new(0, 0, z_far);
	frustum_planes[FAR_FRUSTUM_PLANE].normal.x = 0;
	frustum_planes[FAR_FRUSTUM_PLANE].normal.y = 0;
	frustum_planes[FAR_FRUSTUM_PLANE].normal.z = -1;
}

polygon_t polygon_from_triangle(vec3_t v0, vec3_t v1, vec3_t v2, tex2_t t0, tex2_t t1, tex2_t t2) {
    polygon_t result = {
        .vertices = {v0, v1, v2},
        .tex_coords = {t0, t1, t2},
        .num_vertices = 3
    };

    return result;
}

void triangles_from_polygon(polygon_t* polygon, triangle_t triangles[], int* num_triangles) {
    for (int i = 0; i < polygon->num_vertices - 2; i++) {
        int index0 = 0;
        int index1 = i + 1;
        int index2 = i + 2;

        // Load vertices
        triangles[i].points[0] = vec4_from_vec3(polygon->vertices[index0]);
        triangles[i].points[1] = vec4_from_vec3(polygon->vertices[index1]);
        triangles[i].points[2] = vec4_from_vec3(polygon->vertices[index2]);

        // Load texture coordinates
        triangles[i].tex_coords[0] = polygon->tex_coords[index0];
        triangles[i].tex_coords[1] = polygon->tex_coords[index1];
        triangles[i].tex_coords[2] = polygon->tex_coords[index2];
    }

    *num_triangles = polygon->num_vertices - 2;
}

float float_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void clip_polygon_against_plane(polygon_t* polygon, int plane) {
    vec3_t plane_point = frustum_planes[plane].point;
    vec3_t plane_normal = frustum_planes[plane].normal;

    // Declare an array of vertices inside the plane which will assigned to the polygon at the end
    vec3_t inside_vertices[MAX_NUM_POLY_VERTICES];
    tex2_t inside_tex_coords[MAX_NUM_POLY_VERTICES];
    int num_inside_vertices = 0;

    // initialize current vertex with first polygon vertex and texture coordinate
    vec3_t* current_vertex = &polygon->vertices[0];
    tex2_t* current_tex_coord = &polygon->tex_coords[0];


    // initialize previous vertex with last polygon vertex and texture coordinate
    vec3_t* previous_vertex = &polygon->vertices[polygon->num_vertices - 1];
    tex2_t* previous_tex_coord = &polygon->tex_coords[polygon->num_vertices - 1];

    // initialize current and previous dot products 
    float current_dot = 0;
    float previous_dot = vec3_dot(vec3_sub(*previous_vertex, plane_point), plane_normal);

    // Loop through all polygon vertices to find vertices within the plane
    while (current_vertex != &polygon->vertices[polygon->num_vertices]) {
        current_dot = vec3_dot(vec3_sub(*current_vertex, plane_point), plane_normal);

        // Check if vertex changes from being inside to outside the plane or vice versa
        if (current_dot * previous_dot < 0) {
            // Find interpolation value t
            float t = previous_dot / (previous_dot - current_dot);

            // Calculate intersection point using lerp formula
            vec3_t intersection_point = {
                .x = float_lerp(previous_vertex->x, current_vertex->x, t),
                .y = float_lerp(previous_vertex->y, current_vertex->y, t),
                .z = float_lerp(previous_vertex->z, current_vertex->z, t)
            }; 

            // Calculate U and V texture coordinate using lerp formula
            tex2_t interpolated_tex_coord = {
                .u = float_lerp(previous_tex_coord->u, current_tex_coord->u, t),
                .v = float_lerp(previous_tex_coord->v, current_tex_coord->v, t)
            };

            // Save new vertex and texture coordinate to list of inside vertices
            inside_vertices[num_inside_vertices] = intersection_point;
            inside_tex_coords[num_inside_vertices] = interpolated_tex_coord;
            num_inside_vertices++;
        }
        // Check if current vertex is inside the plane
        if (current_dot > 0) {
            inside_vertices[num_inside_vertices] = *current_vertex;
            inside_tex_coords[num_inside_vertices] = *current_tex_coord;
            num_inside_vertices++;
        }

        // Move to next vertex
        previous_vertex = current_vertex;
        previous_dot = current_dot;
        previous_tex_coord = current_tex_coord;
        current_vertex++;
        current_tex_coord++;
    }
    
    // Copy inside vertices and texture coordinates over to polygon
    for (int i = 0; i < num_inside_vertices; i++) {
        polygon->vertices[i] = inside_vertices[i];
        polygon->tex_coords[i] = inside_tex_coords[i];
    } 
    polygon->num_vertices = num_inside_vertices;
}

void clip_polygon(polygon_t* polygon) {
    clip_polygon_against_plane(polygon, LEFT_FRUSTUM_PLANE);
    clip_polygon_against_plane(polygon, RIGHT_FRUSTUM_PLANE);
    clip_polygon_against_plane(polygon, TOP_FRUSTUM_PLANE);
    clip_polygon_against_plane(polygon, BOTTOM_FRUSTUM_PLANE);
    clip_polygon_against_plane(polygon, NEAR_FRUSTUM_PLANE);
    clip_polygon_against_plane(polygon, FAR_FRUSTUM_PLANE);
}
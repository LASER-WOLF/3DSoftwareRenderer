#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"
#include "upng.h"

typedef struct {
	vec3_t* vertices;   // Mesh dynamic array of verts
	face_t* faces;      // Mesh dynamic array of faces
	upng_t* texture;	// Mesh PNG texture pointer
	vec3_t scale;       // Mesh scale with x, y and z values
	vec3_t rotation;    // Mesh rotation with x, y and z values
	vec3_t translation; // Mesh translation with x, y and z values
} mesh_t;

void load_mesh_obj_data(mesh_t* mesh, char* obj_filename);
void load_mesh_png_data(mesh_t* mesh, char* png_filename);

void load_mesh(char* obj_filename, char* png_filename, vec3_t scale, vec3_t translation, vec3_t rotation);

mesh_t* get_mesh(int mesh_index);
int get_num_meshes(void);

void rotate_mesh_x(int mesh_index, float angle);
void rotate_mesh_y(int mesh_index, float angle);
void rotate_mesh_z(int mesh_index, float angle);

void free_meshes(void);

#endif
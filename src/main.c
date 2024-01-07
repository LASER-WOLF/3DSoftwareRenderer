#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#define SDL_MAIN_HANDLED // TODO: REMOVE THIS LINE?
#include <SDL2/SDL.h>
#include "array.h"
#include "display.h"
#include "mesh.h"
#include "triangle.h"
#include "vector.h"

enum cull_method {
	CULL_NONE,
	CULL_BACKFACE
} cull_method;

enum render_method {
	RENDER_WIRE,
	RENDER_WIRE_VERTEX,
	RENDER_FILL_TRIANGLE,
	RENDER_FILL_TRIANGLE_WIRE
} render_method;

/////////////////////////////////////////////////////////////////////////////////////
// Array of triangles that should be rendered frame by frame
/////////////////////////////////////////////////////////////////////////////////////
triangle_t* triangles_to_render = NULL;

/////////////////////////////////////////////////////////////////////////////////////
// Global variables for execution status and game loop
/////////////////////////////////////////////////////////////////////////////////////
bool is_running = false;
int previous_frame_time = 0;

vec3_t camera_position = { .x = 0, .y = 0, .z = 0 };
float fov_factor = 640;

/////////////////////////////////////////////////////////////////////////////////////
// Setup function to initalize variables and game objects
/////////////////////////////////////////////////////////////////////////////////////
void setup(void) {
	// Initialize render mode and culling method
	render_method = RENDER_WIRE;
	cull_method = CULL_BACKFACE;

	// Allocate the required bytes in memory to hold the color buffer
	color_buffer = (uint32_t*)malloc(sizeof(uint32_t) * window_width * window_height);

	// Creating a SDL texture that is used to display the color buffer
	color_buffer_texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);

	// Loads the cube values in the mesh data structure
	load_cube_mesh_data();
	//load_obj_file_data("./assets/cube.obj");
}

/////////////////////////////////////////////////////////////////////////////////////
// Poll system events and handle keyboard input
/////////////////////////////////////////////////////////////////////////////////////
void process_input(void) {
	SDL_Event event;
	SDL_PollEvent(&event);
	switch(event.type) {
		case SDL_QUIT:
			is_running = false;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				is_running = false;
			if (event.key.keysym.sym == SDLK_1)
				render_method = RENDER_WIRE_VERTEX;
			if (event.key.keysym.sym == SDLK_2)
				render_method = RENDER_WIRE;
			if (event.key.keysym.sym == SDLK_3)
				render_method = RENDER_FILL_TRIANGLE;
			if (event.key.keysym.sym == SDLK_4)
				render_method = RENDER_FILL_TRIANGLE_WIRE;
			if (event.key.keysym.sym == SDLK_c)
				cull_method = CULL_BACKFACE;
			if (event.key.keysym.sym == SDLK_d)
				cull_method = CULL_NONE;
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Function that receives a 3D vector and returns a projected 2D point
/////////////////////////////////////////////////////////////////////////////////////
vec2_t project(vec3_t point) {
	vec2_t projected_point = {
		.x = (fov_factor * point.x) / point.z,
		.y = (fov_factor * point.y) / point.z
	};
	return projected_point;
}

/////////////////////////////////////////////////////////////////////////////////////
// Update function frame by frame with a fixed time step
/////////////////////////////////////////////////////////////////////////////////////
void update(void) {	
	// Cap framerate at target framerate
	int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

	if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
		SDL_Delay(time_to_wait);
	}
	
	previous_frame_time = SDL_GetTicks();

	// Initialize the array of triangles to render
	triangles_to_render = NULL;

	mesh.rotation.x += 0.01;
	mesh.rotation.y += 0.01;
	mesh.rotation.z += 0.01;

	// Loop all triangle faces of our mesh
	int num_faces = array_length(mesh.faces);
	for (int i = 0; i < num_faces; i++) {
		face_t mesh_face = mesh.faces[i];
		
		vec3_t face_vertices[3];
		face_vertices[0] = mesh.vertices[mesh_face.a - 1];
		face_vertices[1] = mesh.vertices[mesh_face.b - 1];
		face_vertices[2] = mesh.vertices[mesh_face.c - 1];

		vec3_t transformed_vertices[3];

		// Loop all three vertices of this current face and apply transformations
		for (int j = 0; j < 3; j++) {
			vec3_t transformed_vertex = face_vertices[j];
			
			transformed_vertex = vec3_rotate_x(transformed_vertex, mesh.rotation.x);
			transformed_vertex = vec3_rotate_y(transformed_vertex, mesh.rotation.y);
			transformed_vertex = vec3_rotate_z(transformed_vertex, mesh.rotation.z);
			
			// Translate the vertex away from the camera
			transformed_vertex.z += 5;

			// Save transformed vertex in the array of transformed vertices
			transformed_vertices[j] = transformed_vertex;
		}

		if (cull_method == CULL_BACKFACE) {
			// Backface culling
			vec3_t vector_a = transformed_vertices[0]; /*   A   */
			vec3_t vector_b = transformed_vertices[1]; /*  / \  */
			vec3_t vector_c = transformed_vertices[2]; /* C---B */

			// Get the vector subtraction of B-A and C-A
			vec3_t vector_ab = vec3_sub(vector_b, vector_a);
			vec3_t vector_ac = vec3_sub(vector_c, vector_a);
			vec3_normalize(&vector_ab);
			vec3_normalize(&vector_ac);

			// Compute the face normal using cross product to find the perpendicular
			vec3_t normal = vec3_cross(vector_ab, vector_ac);
			vec3_normalize(&normal);

			// Find the vector between a point in the triangle (A) and the camera origin
			vec3_t camera_ray = vec3_sub(camera_position, vector_a);

			// Check normal alignment
			float dot_normal_camera = vec3_dot(normal, camera_ray);

			// Bypass the triangles that are looking away from the camera
			if (dot_normal_camera < 0) {
				continue;
			}
		}

		// Project
		vec2_t projected_points[3];
		for (int j = 0; j < 3; j++) {
			// Project the current vertex
			projected_points[j] = project(transformed_vertices[j]);

			// Scale and translate the projected points to the middle of the screen
			projected_points[j].x += (window_width / 2);
			projected_points[j].y += (window_height / 2);
		}

		// Calculate the average deptj for each face based on the verts after transformation
		float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z + transformed_vertices[2].z) / 3.0;

		triangle_t projected_triangle = {
			.points = {
				{ projected_points[0].x, projected_points[0].y },
				{ projected_points[1].x, projected_points[1].y },
				{ projected_points[2].x, projected_points[2].y }
			},
			.color = mesh_face.color,
			.avg_depth = avg_depth
		};

		// Save the projected triangle in the array of triangles to render
		array_push(triangles_to_render, projected_triangle);
	}

	// Sort the triangles to render by their avg_depth using painters algorithm
	int num_triangles = array_length(triangles_to_render);
	for (int i = 0; i < num_triangles; i++) {
		for (int j = i; j < num_triangles; j++) {
			if (triangles_to_render[i].avg_depth < triangles_to_render[j].avg_depth) {
				triangle_t temp = triangles_to_render[i];
				triangles_to_render[i] = triangles_to_render[j];
				triangles_to_render[j] = temp;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Render function to draw objects on the display
/////////////////////////////////////////////////////////////////////////////////////
void render(void) {
	draw_grid();

	// Loop all projected triangles and render them
	int num_triangles = array_length(triangles_to_render);
	for (int i = 0; i < num_triangles; i++) {
		triangle_t triangle = triangles_to_render[i];

		// Draw filled triangle
		if (render_method == RENDER_FILL_TRIANGLE || render_method ==RENDER_FILL_TRIANGLE_WIRE) {
			draw_filled_triangle(
				triangle.points[0].x, triangle.points[0].y, 
				triangle.points[1].x, triangle.points[1].y, 
				triangle.points[2].x, triangle.points[2].y, 
				triangle.color
			);
		}

		// Draw unfilled triangle
		if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE) {
			draw_triangle(
				triangle.points[0].x, triangle.points[0].y, 
				triangle.points[1].x, triangle.points[1].y, 
				triangle.points[2].x, triangle.points[2].y, 
				0xFFDDDDDD
			);
		}

		// Draw vertex points
		if (render_method == RENDER_WIRE_VERTEX) {
			draw_rect(triangle.points[0].x, triangle.points[0].y, 3, 3, 0xFFDD0000);
			draw_rect(triangle.points[1].x, triangle.points[1].y, 3, 3, 0xFFDD0000);
			draw_rect(triangle.points[2].x, triangle.points[2].y, 3, 3, 0xFFDD0000);
		}
	}

	// Clear the array of triangles to render every frame loop
	array_free(triangles_to_render);

	render_color_buffer();
	
	clear_color_buffer(0xFF111111);

	SDL_RenderPresent(renderer);
}

/////////////////////////////////////////////////////////////////////////////////////
// Free memory taht was dunamicaly allocated by the program
/////////////////////////////////////////////////////////////////////////////////////
void free_resources(void) {
	free(color_buffer);
	array_free(mesh.vertices);
	array_free(mesh.faces);
}

/////////////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////////////
int main(void) {
	is_running = initialize_window();
	
	setup();

	while(is_running) {
		process_input();
		update();
		render();
	}

	destroy_window();
	free_resources();

	return 0;
}
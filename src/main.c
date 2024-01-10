#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#define SDL_MAIN_HANDLED // TODO: REMOVE THIS LINE?
#include <SDL2/SDL.h>
#include "upng.h"
#include "array.h"
#include "display.h"
#include "vector.h"
#include "matrix.h"
#include "light.h"
#include "triangle.h"
#include "texture.h"
#include "mesh.h"

enum cull_method {
	CULL_NONE,
	CULL_BACKFACE
} cull_method;

enum render_method {
	RENDER_WIRE,
	RENDER_WIRE_VERTEX,
	RENDER_FILL_TRIANGLE,
	RENDER_FILL_TRIANGLE_WIRE,
	RENDER_TEXTURED,
	RENDER_TEXTURED_WIRE
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
mat4_t proj_matrix;

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
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
	);

	// Initialize the perspective projection matrix
	float fov = M_PI / 3.0;                      // the same as 180 / 3 or 60deg
	float aspect = (float)window_height / (float)window_width;
	float znear = 0.1;
	float zfar = 100.0;
	proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);
	
	// Loads the cube values in the mesh data structure
	load_cube_mesh_data();
	//load_obj_file_data("./assets/f22.obj");

	// Load thhe texture information from an external PNG file
	load_png_texture_data("./assets/cube.png");

	// Manually load the hardcoded texture data from the static array
	//mesh_texture = (uint32_t*) REDBRICK_TEXTURE;
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
			if (event.key.keysym.sym == SDLK_5)
				render_method = RENDER_TEXTURED;
			if (event.key.keysym.sym == SDLK_6)
				render_method = RENDER_TEXTURED_WIRE;
			if (event.key.keysym.sym == SDLK_c)
				cull_method = CULL_BACKFACE;
			if (event.key.keysym.sym == SDLK_d)
				cull_method = CULL_NONE;
			break;
	}
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

	// Change the mesh scale / rotation values per animation frame
	//mesh.scale.x += 0.002;
	//mesh.scale.y += 0.001;
	//mesh.scale.z += 0.001;
	//mesh.rotation.x += 0.003;
	mesh.rotation.y += 0.001;
	//mesh.rotation.z += 0.01;
	//mesh.translation.x += 0.01;
	mesh.translation.z = 5.0;


	// Create a scale, rotation and translation matrices that will be used to multiply the mesh vertices
	mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
	mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
	mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
	mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);
	mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);

	// Loop all triangle faces of our mesh
	int num_faces = array_length(mesh.faces);
	for (int i = 0; i < num_faces; i++) {
		face_t mesh_face = mesh.faces[i];
		
		vec3_t face_vertices[3];
		face_vertices[0] = mesh.vertices[mesh_face.a - 1];
		face_vertices[1] = mesh.vertices[mesh_face.b - 1];
		face_vertices[2] = mesh.vertices[mesh_face.c - 1];

		vec4_t transformed_vertices[3];

		// Loop all three vertices of this current face and apply transformations
		for (int j = 0; j < 3; j++) {
			vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);
			
			// Create a World Matrix combining scale, rotation and translation matrices
			mat4_t world_matrix = mat4_identity();
			
			// Order matters: First scale, then rotate, then translate. [T]*[R]*[S]*v
			world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
			world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
			world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

			// Multiply the world matrix by the original vector
			transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

			// Save transformed vertex in the array of transformed vertices
			transformed_vertices[j] = transformed_vertex;
		}

		
		// Backface culling test
		vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
		vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */
		vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

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

		if (cull_method == CULL_BACKFACE) {
			// Bypass the triangles that are looking away from the camera
			if (dot_normal_camera < 0) {
				continue;
			}
		}
		
		// Project
		vec4_t projected_points[3];
		for (int j = 0; j < 3; j++) {
			// Project the current vertex
			projected_points[j] = mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

			// Scale into the view
			projected_points[j].x *= (window_width / 2.0);
			projected_points[j].y *= (window_height / 2.0);

			// Invert the y values to account for flipped screen y coordinate
			projected_points[j].y *= -1;

			// Translate the projected points to the middle of the screen
			projected_points[j].x += (window_width / 2.0);
			projected_points[j].y += (window_height / 2.0);
		}

		// Calculate the average depth for each face based on the verts after transformation
		float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z + transformed_vertices[2].z) / 3.0;

		// Calculate the shade intensity based on how aligned the face normal and the inverse of the light ray
		float light_intensity_factor = -vec3_dot(normal, light.direction);

		// Calculate the triangle color based on the light angle
		uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

		triangle_t projected_triangle = {
			.points = {
				{ projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w },
				{ projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w },
				{ projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w }
			},
			.texcoords = {
				{ mesh_face.a_uv.u, mesh_face.a_uv.v },
				{ mesh_face.b_uv.u, mesh_face.b_uv.v },
				{ mesh_face.c_uv.u, mesh_face.c_uv.v }
			},
			.color = triangle_color,
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

        // Draw textured triangle
        if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE) {
            draw_textured_triangle(
                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
                mesh_texture
            );
        }

		// Draw unfilled triangle
		if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE || render_method == RENDER_TEXTURED_WIRE
			) {
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
	upng_free(png_texture);
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
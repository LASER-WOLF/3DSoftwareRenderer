#include "display.h"
#include "swap.h"
#include "triangle.h"

///////////////////////////////////////////////////////////////////////////////
// Return the normal vector of a triangle face
///////////////////////////////////////////////////////////////////////////////
vec3_t get_triangle_normal(vec4_t vertices[3]) {
	// Get individual vectors from A, B and C vertices to compute normal
	vec3_t vector_a = vec3_from_vec4(vertices[0]); /*   A   */
	vec3_t vector_b = vec3_from_vec4(vertices[1]); /*  / \  */
	vec3_t vector_c = vec3_from_vec4(vertices[2]); /* C---B */

	// Get the vector subtraction of B-A and C-A
	vec3_t vector_ab = vec3_sub(vector_b, vector_a);
	vec3_t vector_ac = vec3_sub(vector_c, vector_a);
	vec3_normalize(&vector_ab);
	vec3_normalize(&vector_ac);

	// Compute the face normal using cross product to find the perpendicular
	vec3_t normal = vec3_cross(vector_ab, vector_ac);
	vec3_normalize(&normal);

	return normal;
}

///////////////////////////////////////////////////////////////////////////////
// Return the barycentric weights alpha, beta, and gamma for point p
///////////////////////////////////////////////////////////////////////////////
//
//         (B)
//         /|\
//        / | \
//       /  |  \
//      /  (P)  \
//     /  /   \  \
//    / /       \ \
//   //           \\
//  (A)------------(C)
//
///////////////////////////////////////////////////////////////////////////////
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
	// Find the vectors between the vertices ABC and point p
	vec2_t ac = vec2_sub(c, a);
	vec2_t ab = vec2_sub(b, a);
	vec2_t ap = vec2_sub(p, a);
	vec2_t pc = vec2_sub(c, p);
	vec2_t pb = vec2_sub(b, p);

	// Compute the area of the full parallegram/triangle ABC using 2D cross product
	float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x); // || AC x AB ||

	// Alpha is the area of the small parallelogram/triangle PBC divided by the area of the full parallelogram/triangle ABC
	float alpha = (pc.x * pb.y - pc.y * pb.x) / area_parallelogram_abc;

	// Beta is the area of the small parallelogram/triangle APC divided by the area of the full parallelogram/triangle ABC
	float beta = (ac.x * ap.y - ac.y * ap.x) / area_parallelogram_abc;

	// Weight gamma is easily found since barycentric coordinates always add up to 1.0
	float gamma = 1 - alpha - beta;

	vec3_t weights = { alpha, beta, gamma };
	return weights;
}

///////////////////////////////////////////////////////////////////////////////
// Function to draw the colored triangle pixel at position x and y with z_buffer
///////////////////////////////////////////////////////////////////////////////
void draw_triangle_pixel(
	int x, int y, uint32_t color,
	vec4_t point_a, vec4_t point_b, vec4_t point_c
) {
	vec2_t point_p = { x, y };
	vec2_t a = vec2_from_vec4(point_a);
	vec2_t b = vec2_from_vec4(point_b);
	vec2_t c = vec2_from_vec4(point_c);
	vec3_t weights = barycentric_weights(a, b, c, point_p);

	float alpha = weights.x;
	float beta = weights.y;
	float gamma = weights.z;

	// Interpolate the values of 1/w for the current pixel
	float interpolated_reciprocal_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;

	// Adjust 1/w so the pixels that are closer to the camera have smaller values
	interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w;

	// Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
	if (interpolated_reciprocal_w < get_zbuffer_at(x, y)) {

		// Draw a pixel at position (x,y) with the color that comes from the mapped texture
		draw_pixel(x, y, color);

		// Update the z-buffer value with the 1/w of this current pixel
		update_zbuffer_at(x, y, interpolated_reciprocal_w);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Function to draw the textured pixel at position x and y using interpolation
///////////////////////////////////////////////////////////////////////////////
void draw_triangle_texel(
	int x, int y, 
	float light, uint32_t* texture_buffer,
	int texture_width, int texture_height,
	vec4_t point_a, vec4_t point_b, vec4_t point_c,
	tex2_t a_uv, tex2_t b_uv, tex2_t c_uv
) {
	vec2_t point_p = { x, y };
	vec2_t a = vec2_from_vec4(point_a);
	vec2_t b = vec2_from_vec4(point_b);
	vec2_t c = vec2_from_vec4(point_c);
	vec3_t weights = barycentric_weights(a, b, c, point_p);

	float alpha = weights.x;
	float beta = weights.y;
	float gamma = weights.z;

	// Variables to store the interpolated values of U, V, and also 1/W for the current pixel
	float interpolated_u;
	float interpolated_v;
	float interpolated_reciprocal_w;

	// Perform the interpolation of all U/w and V/w values using barycentric weights and a factor of 1/w
	interpolated_u = (a_uv.u / point_a.w) * alpha + (b_uv.u / point_b.w) * beta + (c_uv.u / point_c.w) * gamma;
	interpolated_v = (a_uv.v / point_a.w) * alpha + (b_uv.v / point_b.w) * beta + (c_uv.v / point_c.w) * gamma;

	// Also interpolate the values of 1/w for the current pixel
	interpolated_reciprocal_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;;
	
	// Divide back both interpolated values by 1/w
	interpolated_u /= interpolated_reciprocal_w;
	interpolated_v /= interpolated_reciprocal_w;

	// Map the UV coordinate to the full texture width and height
	int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width;
	int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;

	// Adjust 1/w so the pixels that are closer to the camera have smaller values
	interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w;

	// Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
	if (interpolated_reciprocal_w < get_zbuffer_at(x, y)) {
		

		uint32_t color = texture_buffer[(texture_width * tex_y) + tex_x];

		// Calculate the triangle color based on the light angle
		uint32_t color_with_light = apply_light_intensity(color, light);

		// Draw a pixel at position (x,y) with the color that comes from the mapped texture
		draw_pixel(x, y, color_with_light);

		// Update the z-buffer value with the 1/w of this current pixel
		update_zbuffer_at(x, y, interpolated_reciprocal_w);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Draw a triangle using three raw line calls
///////////////////////////////////////////////////////////////////////////////
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
	draw_line(x0, y0, x1, y1, color);
	draw_line(x1, y1, x2, y2, color);
	draw_line(x2, y2, x0, y0, color);
}

///////////////////////////////////////////////////////////////////////////////
// Draw a filled triangle with the flat-top/flat-bottom method
// We split the original triangle in two, half flat-bottom and half flat-top
///////////////////////////////////////////////////////////////////////////////
//
//          (x0,y0)
//            / \
//           /   \
//          /     \
//         /       \
//        /         \
//   (x1,y1)---------\
//       \_           \
//          \_         \
//             \_       \
//                \_     \
//                   \    \
//                     \_  \
//                        \_\
//                           \
//                         (x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_filled_triangle(
	int x0, int y0, float z0, float w0,
	int x1, int y1, float z1, float w1, 
	int x2, int y2, float z2, float w2,
	float light, uint32_t color) {
	
	// Calculate the triangle color based on the light angle
	uint32_t color_with_light = apply_light_intensity(color, light);

	// We need to sort the vertices by y-coordinate ascending (y0 < y1 < y2)
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

	// Create vector points and texture coords after we sort the vertices
	vec4_t point_a = { x0, y0, z0, w0 };
	vec4_t point_b = { x1, y1, z1, w1 };
	vec4_t point_c = { x2, y2, z2, w2 };

	///////////////////////////////////////////////////////
	// Render the upper part of the triangle (flat-bottom)
	///////////////////////////////////////////////////////
	float inv_slope_1 = 0;
	float inv_slope_2 = 0;

	if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
	if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

	if (y1 - y0 != 0) {
		for (int y = y0; y <= y1; y++) {
			int x_start = x1 + (y - y1) * inv_slope_1;
			int x_end = x0 + (y - y0) * inv_slope_2;

			if (x_end < x_start) {
				int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
			}

			for (int x = x_start; x < x_end; x++) {
				// Draw our pixel with the color that comes from the texture
				draw_triangle_pixel(x, y, color_with_light, point_a, point_b, point_c);
			}
		}
	}

	///////////////////////////////////////////////////////
	// Render the bottom part of the triangle (flat-top)
	///////////////////////////////////////////////////////
	inv_slope_1 = 0;
	inv_slope_2 = 0;

	if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
	if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

	if (y2 - y1 != 0) {
		for (int y = y1; y <= y2; y++) {
			int x_start = x1 + (y - y1) * inv_slope_1;
			int x_end = x0 + (y - y0) * inv_slope_2;

			if (x_end < x_start) {
				int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
			}

			for (int x = x_start; x < x_end; x++) {
				// Draw our pixel with the color that comes from the texture
				draw_triangle_pixel(x, y, color_with_light, point_a, point_b, point_c);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Draw a textured triangle based on a texture array of colors.
// We split the original triangle in two, half flat-bottom and half flat-top.
///////////////////////////////////////////////////////////////////////////////
//
//        v0
//        /\
//       /  \
//      /    \
//     /      \
//   v1--------\
//     \_       \
//        \_     \
//           \_   \
//              \_ \
//                 \\
//                   \
//                    v2
//
///////////////////////////////////////////////////////////////////////////////
void draw_textured_triangle(
	int x0, int y0, float z0, float w0, float u0, float v0,
	int x1, int y1, float z1, float w1, float u1, float v1,
	int x2, int y2, float z2, float w2, float u2, float v2,
	float light, upng_t* texture
) {
	// Get the mesh texture width and height dimensions
	int texture_width = upng_get_width(texture);
	int texture_height = upng_get_height(texture);
	
	// Create texture buffer from the mesh texture
	uint32_t* texture_buffer = (uint32_t*)upng_get_buffer(texture);

	// We need to sort the vertices by y-coordinate ascending (y0 < y1 < y2)
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

	// Flip the V component to account for inverted UV-coordinates
	v0 = 1.0 - v0;
	v1 = 1.0 - v1;
	v2 = 1.0 - v2;

	// Create vector points and texture coords after we sort the vertices
	vec4_t point_a = { x0, y0, z0, w0 };
	vec4_t point_b = { x1, y1, z1, w1 };
	vec4_t point_c = { x2, y2, z2, w2 };
	tex2_t a_uv = { u0, v0 };
	tex2_t b_uv = { u1, v1 };
	tex2_t c_uv = { u2, v2 };

	///////////////////////////////////////////////////////
	// Render the upper part of the triangle (flat-bottom)
	///////////////////////////////////////////////////////
	float inv_slope_1 = 0;
	float inv_slope_2 = 0;

	if (y1 - y0 != 0) inv_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
	if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

	if (y1 - y0 != 0) {
		for (int y = y0; y <= y1; y++) {
			int x_start = x1 + (y - y1) * inv_slope_1;
			int x_end = x0 + (y - y0) * inv_slope_2;

			if (x_end < x_start) {
				int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
			}

			for (int x = x_start; x < x_end; x++) {
				// Draw our pixel with the color that comes from the texture
				draw_triangle_texel(x, y, light, texture_buffer, texture_width, texture_height, point_a, point_b, point_c, a_uv, b_uv, c_uv);
			}
		}
	}

	///////////////////////////////////////////////////////
	// Render the bottom part of the triangle (flat-top)
	///////////////////////////////////////////////////////
	inv_slope_1 = 0;
	inv_slope_2 = 0;

	if (y2 - y1 != 0) inv_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
	if (y2 - y0 != 0) inv_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

	if (y2 - y1 != 0) {
		for (int y = y1; y <= y2; y++) {
			int x_start = x1 + (y - y1) * inv_slope_1;
			int x_end = x0 + (y - y0) * inv_slope_2;

			if (x_end < x_start) {
				int_swap(&x_start, &x_end); // swap if x_start is to the right of x_end
			}

			for (int x = x_start; x < x_end; x++) {
				// Draw our pixel with the color that comes from the texture
				draw_triangle_texel(x, y, light, texture_buffer, texture_width, texture_height, point_a, point_b, point_c, a_uv, b_uv, c_uv);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Triangle Rasterizer
// (1/3) A Parallel Algorithm for Polygon Rasterization (Juan Pineda): https://www.cs.drexel.edu/~deb39/Classes/Papers/comp175-06-pineda.pdf
// (2/3) Optimizing the basic rasterizer (Fabian Giesen): https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
// (3/3) A fast and precise triangle rasterizer (Kristoffer Dyrkorn): https://kristoffer-dyrkorn.github.io/triangle-rasterizer/
///////////////////////////////////////////////////////////////////////////////
// Implementation (1/5)
// Loop through bounding box and check if pixel is inside triangle
///////////////////////////////////////////////////////////////////////////////
// int edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { b->x - a->x, b->y - a->y };
//   vec2_t ap = { p->x - a->x, p->y - a->y };
//   return ab.x * ap.y - ab.y * ap.x;
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2, uint32_t color) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = MIN(MIN(v0.x, v1.x), v2.x);
//   int y_min = MIN(MIN(v0.y, v1.y), v2.y);
//   int x_max = MAX(MAX(v0.x, v1.x), v2.x);
//   int y_max = MAX(MAX(v0.y, v1.y), v2.y);

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     for (int x = x_min; x <= x_max; x++) {
//       vec2_t p = { x, y };
//       int w0 = edge_cross(&v1, &v2, &p);
//       int w1 = edge_cross(&v2, &v0, &p);
//       int w2 = edge_cross(&v0, &v1, &p);
      
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;

//       if (is_inside) {
//         draw_pixel(x, y, color);
//       }
//     }
//   }
// }
///////////////////////////////////////////////////////////////////////////////
// Implementation (2/5)
// Added Top-Left Rasterization Rule to avoid overdraw of pixels
///////////////////////////////////////////////////////////////////////////////
// bool is_top_left(vec2_t* start, vec2_t* end) {
//   vec2_t edge = { end->x - start->x, end->y - start->y };
//   bool is_top_edge = edge.y == 0 && edge.x > 0;
//   bool is_left_edge = edge.y < 0;
//   return is_top_edge || is_left_edge;
// }

// int edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { b->x - a->x, b->y - a->y };
//   vec2_t ap = { p->x - a->x, p->y - a->y };
//   return ab.x * ap.y - ab.y * ap.x;
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2, uint32_t color) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = MIN(MIN(v0.x, v1.x), v2.x);
//   int y_min = MIN(MIN(v0.y, v1.y), v2.y);
//   int x_max = MAX(MAX(v0.x, v1.x), v2.x);
//   int y_max = MAX(MAX(v0.y, v1.y), v2.y);

//   // Fill convention (top-left rasterization rule)
//   int bias0 = is_top_left(&v1, &v2) ? 0 : -1;
//   int bias1 = is_top_left(&v2, &v0) ? 0 : -1;
//   int bias2 = is_top_left(&v0, &v1) ? 0 : -1;

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     for (int x = x_min; x <= x_max; x++) {
//       vec2_t p = { x, y };
//       int w0 = edge_cross(&v1, &v2, &p) + bias0;
//       int w1 = edge_cross(&v2, &v0, &p) + bias1;
//       int w2 = edge_cross(&v0, &v1, &p) + bias2;
      
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;

//       if (is_inside) {
//         draw_pixel(x, y, color);
//       }
//     }
//   }
// }
///////////////////////////////////////////////////////////////////////////////
// Implementation (3/5)
// Added color interpolation with Barycentric coordinates
///////////////////////////////////////////////////////////////////////////////
// bool is_top_left(vec2_t* start, vec2_t* end) {
//   vec2_t edge = { end->x - start->x, end->y - start->y };
//   bool is_top_edge = edge.y == 0 && edge.x > 0;
//   bool is_left_edge = edge.y < 0;
//   return is_top_edge || is_left_edge;
// }

// int edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { b->x - a->x, b->y - a->y };
//   vec2_t ap = { p->x - a->x, p->y - a->y };
//   return ab.x * ap.y - ab.y * ap.x;
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = MIN(MIN(v0.x, v1.x), v2.x);
//   int y_min = MIN(MIN(v0.y, v1.y), v2.y);
//   int x_max = MAX(MAX(v0.x, v1.x), v2.x);
//   int y_max = MAX(MAX(v0.y, v1.y), v2.y);

//   // Compute the area of the entire triangle/parallelogram
//   float area = edge_cross(&v0, &v1, &v2);

//   // Fill convention (top-left rasterization rule)
//   int bias0 = is_top_left(&v1, &v2) ? 0 : -1;
//   int bias1 = is_top_left(&v2, &v0) ? 0 : -1;
//   int bias2 = is_top_left(&v0, &v1) ? 0 : -1;

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     for (int x = x_min; x <= x_max; x++) {
//       vec2_t p = { x, y };
//       int w0 = edge_cross(&v1, &v2, &p) + bias0;
//       int w1 = edge_cross(&v2, &v0, &p) + bias1;
//       int w2 = edge_cross(&v0, &v1, &p) + bias2;
      
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;

//       if (is_inside) {
//         float alpha = w0 / area;
//         float beta  = w1 / area;
//         float gamma = w2 / area;

//         int a = 0xFF;
//         int r = (alpha) * colors[0].r + (beta) * colors[1].r + (gamma) * colors[2].r;
//         int g = (alpha) * colors[0].g + (beta) * colors[1].g + (gamma) * colors[2].g;
//         int b = (alpha) * colors[0].b + (beta) * colors[1].b + (gamma) * colors[2].b;

//         uint32_t interp_color = 0x00000000;
//         interp_color = (interp_color | a) << 8;
//         interp_color = (interp_color | b) << 8;
//         interp_color = (interp_color | g) << 8;
//         interp_color = (interp_color | r);

//         draw_pixel(x, y, interp_color);
//       }
//     }
//   }
// }
///////////////////////////////////////////////////////////////////////////////
// Implementation (4/5)
// Added constant increments per row / column to speed up the function
///////////////////////////////////////////////////////////////////////////////
// bool is_top_left(vec2_t* start, vec2_t* end) {
//   vec2_t edge = { end->x - start->x, end->y - start->y };
//   bool is_top_edge = edge.y == 0 && edge.x > 0;
//   bool is_left_edge = edge.y < 0;
//   return is_top_edge || is_left_edge;
// }

// int edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { b->x - a->x, b->y - a->y };
//   vec2_t ap = { p->x - a->x, p->y - a->y };
//   return ab.x * ap.y - ab.y * ap.x;
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = MIN(MIN(v0.x, v1.x), v2.x);
//   int y_min = MIN(MIN(v0.y, v1.y), v2.y);
//   int x_max = MAX(MAX(v0.x, v1.x), v2.x);
//   int y_max = MAX(MAX(v0.y, v1.y), v2.y);

//   // Compute the constant delta_s that will be used for the horizontal and vertical steps
//   int delta_w0_col = (v1.y - v2.y);
//   int delta_w1_col = (v2.y - v0.y);
//   int delta_w2_col = (v0.y - v1.y);

//   int delta_w0_row = (v2.x - v1.x);
//   int delta_w1_row = (v0.x - v2.x);
//   int delta_w2_row = (v1.x - v0.x);

//   // Compute the area of the entire triangle/parallelogram
//   float area = edge_cross(&v0, &v1, &v2);

//   // Fill convention (top-left rasterization rule)
//   int bias0 = is_top_left(&v1, &v2) ? 0 : -1;
//   int bias1 = is_top_left(&v2, &v0) ? 0 : -1;
//   int bias2 = is_top_left(&v0, &v1) ? 0 : -1;

//   vec2_t p0 = { x_min, y_min };
//   int w0_row = edge_cross(&v1, &v2, &p0) + bias0;
//   int w1_row = edge_cross(&v2, &v0, &p0) + bias1;
//   int w2_row = edge_cross(&v0, &v1, &p0) + bias2;

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     int w0 = w0_row;
//     int w1 = w1_row;
//     int w2 = w2_row;
//     for (int x = x_min; x <= x_max; x++) {
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;
//       if (is_inside) {
//         float alpha = w0 / area;
//         float beta  = w1 / area;
//         float gamma = w2 / area;

//         int a = 0xFF;
//         int r = (alpha) * colors[0].r + (beta) * colors[1].r + (gamma) * colors[2].r;
//         int g = (alpha) * colors[0].g + (beta) * colors[1].g + (gamma) * colors[2].g;
//         int b = (alpha) * colors[0].b + (beta) * colors[1].b + (gamma) * colors[2].b;

//         uint32_t interp_color = 0x00000000;
//         interp_color = (interp_color | a) << 8;
//         interp_color = (interp_color | b) << 8;
//         interp_color = (interp_color | g) << 8;
//         interp_color = (interp_color | r);

//         draw_pixel(x, y, interp_color);
//       }
//       w0 += delta_w0_col;
//       w1 += delta_w1_col;
//       w2 += delta_w2_col;
//     }
//     w0_row += delta_w0_row;
//     w1_row += delta_w1_row;
//     w2_row += delta_w2_row;
//   }
// }
///////////////////////////////////////////////////////////////////////////////
// Implementation (5/6)
// Added subpixel rasterization by replacing integers with floating-point numbers
// https://github.com/gustavopezzi/triangle-rasterizer-float.git
///////////////////////////////////////////////////////////////////////////////
// bool is_top_left(vec2_t* start, vec2_t* end) {
//   vec2_t edge = { end->x - start->x, end->y - start->y };
//   bool is_top_edge = edge.y == 0 && edge.x > 0;
//   bool is_left_edge = edge.y < 0;
//   return is_top_edge || is_left_edge;
// }

// float edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { b->x - a->x, b->y - a->y };
//   vec2_t ap = { p->x - a->x, p->y - a->y };
//   return ab.x * ap.y - ab.y * ap.x;
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = floor(MIN(MIN(v0.x, v1.x), v2.x));
//   int y_min = floor(MIN(MIN(v0.y, v1.y), v2.y));
//   int x_max = ceil(MAX(MAX(v0.x, v1.x), v2.x));
//   int y_max = ceil(MAX(MAX(v0.y, v1.y), v2.y));

//   // Compute the constant delta_s that will be used for the horizontal and vertical steps
//   float delta_w0_col = (v1.y - v2.y);
//   float delta_w1_col = (v2.y - v0.y);
//   float delta_w2_col = (v0.y - v1.y);
//   float delta_w0_row = (v2.x - v1.x);
//   float delta_w1_row = (v0.x - v2.x);
//   float delta_w2_row = (v1.x - v0.x);

//   // Compute the area of the entire triangle/parallelogram
//   float area = edge_cross(&v0, &v1, &v2);

//   // Fill convention (top-left rasterization rule)
//   float bias0 = is_top_left(&v1, &v2) ? 0 : -0.0001;
//   float bias1 = is_top_left(&v2, &v0) ? 0 : -0.0001;
//   float bias2 = is_top_left(&v0, &v1) ? 0 : -0.0001;

//   vec2_t p0 = { x_min + 0.5f, y_min + 0.5f };
//   float w0_row = edge_cross(&v1, &v2, &p0) + bias0;
//   float w1_row = edge_cross(&v2, &v0, &p0) + bias1;
//   float w2_row = edge_cross(&v0, &v1, &p0) + bias2;

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     float w0 = w0_row;
//     float w1 = w1_row;
//     float w2 = w2_row;
//     for (int x = x_min; x <= x_max; x++) {
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;
//       if (is_inside) {
//         float alpha = w0 / area;
//         float beta  = w1 / area;
//         float gamma = w2 / area;

//         int a = 0xFF;
//         int r = (alpha) * colors[0].r + (beta) * colors[1].r + (gamma) * colors[2].r;
//         int g = (alpha) * colors[0].g + (beta) * colors[1].g + (gamma) * colors[2].g;
//         int b = (alpha) * colors[0].b + (beta) * colors[1].b + (gamma) * colors[2].b;

//         uint32_t interp_color = 0x00000000;
//         interp_color = (interp_color | a) << 8;
//         interp_color = (interp_color | b) << 8;
//         interp_color = (interp_color | g) << 8;
//         interp_color = (interp_color | r);

//         draw_pixel(x, y, interp_color);
//       }
//       w0 += delta_w0_col;
//       w1 += delta_w1_col;
//       w2 += delta_w2_col;
//     }
//     w0_row += delta_w0_row;
//     w1_row += delta_w1_row;
//     w2_row += delta_w2_row;
//   }
// }
///////////////////////////////////////////////////////////////////////////////
// Implementation (6/6)
// Changed floating-point numbers to 16.16 fixed point numbers for better accuracy
// https://github.com/gustavopezzi/triangle-rasterizer-fix16.git
// Requires libfixmath library: https://code.google.com/archive/p/libfixmath/
///////////////////////////////////////////////////////////////////////////////
// bool is_top_left(vec2_t* start, vec2_t* end) {
//   vec2_t edge = { fix16_sub(end->x, start->x), fix16_sub(end->y, start->y) };
//   bool is_top_edge = edge.y == 0 && edge.x > 0;
//   bool is_left_edge = edge.y < 0;
//   return is_left_edge || is_top_edge;
// }

// fix16_t edge_cross(vec2_t* a, vec2_t* b, vec2_t* p) {
//   vec2_t ab = { fix16_sub(b->x, a->x), fix16_sub(b->y, a->y) };
//   vec2_t ap = { fix16_sub(p->x, a->x), fix16_sub(p->y, a->y) };
//   return fix16_sub(fix16_mul(ab.x, ap.y), fix16_mul(ab.y, ap.x));
// }

// void triangle_fill(vec2_t v0, vec2_t v1, vec2_t v2) {
//   // Finds the bounding box with all candidate pixels
//   int x_min = fix16_to_int(MIN(MIN(v0.x, v1.x), v2.x));
//   int y_min = fix16_to_int(MIN(MIN(v0.y, v1.y), v2.y));
//   int x_max = fix16_to_int(MAX(MAX(v0.x, v1.x), v2.x));
//   int y_max = fix16_to_int(MAX(MAX(v0.y, v1.y), v2.y));

//   // Compute the area of the entire triangle/parallelogram
//   fix16_t area = edge_cross(&v0, &v1, &v2);

//   // Compute the constant delta_s that will be used for the horizontal and vertical steps
//   fix16_t delta_w0_col = fix16_sub(v1.y, v2.y);
//   fix16_t delta_w1_col = fix16_sub(v2.y, v0.y);
//   fix16_t delta_w2_col = fix16_sub(v0.y, v1.y);
//   fix16_t delta_w0_row = fix16_sub(v2.x, v1.x);
//   fix16_t delta_w1_row = fix16_sub(v0.x, v2.x);
//   fix16_t delta_w2_row = fix16_sub(v1.x, v0.x);

//   // Rasterization fill convention (top-left rule)
//   fix16_t bias0 = is_top_left(&v1, &v2) ? 0 : -0x00000001;
//   fix16_t bias1 = is_top_left(&v2, &v0) ? 0 : -0x00000001;
//   fix16_t bias2 = is_top_left(&v0, &v1) ? 0 : -0x00000001;

//   // Compute the edge functions for the fist (top-left) point
//   vec2_t p0 = {
//     fix16_from_float(x_min + 0.5f),
//     fix16_from_float(y_min + 0.5f)
//   };
//   fix16_t w0_row = fix16_add(edge_cross(&v1, &v2, &p0), bias0);
//   fix16_t w1_row = fix16_add(edge_cross(&v2, &v0, &p0), bias1);
//   fix16_t w2_row = fix16_add(edge_cross(&v0, &v1, &p0), bias2);

//   // Loop all candidate pixels inside the bounding box
//   for (int y = y_min; y <= y_max; y++) {
//     fix16_t w0 = w0_row;
//     fix16_t w1 = w1_row;
//     fix16_t w2 = w2_row;
    
//     for (int x = x_min; x <= x_max; x++) {
//       bool is_inside = w0 >= 0 && w1 >= 0 && w2 >= 0;
//       if (is_inside) {
//         // Compute the normalized barycentric weights alpha, beta, and gamma
//         float alpha = fix16_to_float(fix16_div(w0, area));
//         float beta = fix16_to_float(fix16_div(w1, area));
//         float gamma = fix16_to_float(fix16_div(w2, area));

//         // Find the new RGB components interpolating vertex values using alpha, beta, and gamma
//         int a = 0xFF;
//         int r = (alpha) * colors[0].r + (beta) * colors[1].r + (gamma) * colors[2].r;
//         int g = (alpha) * colors[0].g + (beta) * colors[1].g + (gamma) * colors[2].g;
//         int b = (alpha) * colors[0].b + (beta) * colors[1].b + (gamma) * colors[2].b;

//         // Combine A, R, G, and B into one final 32-bit color
//         uint32_t interp_color = 0x00000000;
//         interp_color = (interp_color | a) << 8;
//         interp_color = (interp_color | b) << 8;
//         interp_color = (interp_color | g) << 8;
//         interp_color = (interp_color | r);

//         draw_pixel(x, y, interp_color);
//       }
//       // Increment one step to the right
//       w0 = fix16_add(w0, delta_w0_col);
//       w1 = fix16_add(w1, delta_w1_col);
//       w2 = fix16_add(w2, delta_w2_col);
//     }
//     // Increment one row step
//     w0_row = fix16_add(w0_row, delta_w0_row);
//     w1_row = fix16_add(w1_row, delta_w1_row);
//     w2_row = fix16_add(w2_row, delta_w2_row);
//   }
// }
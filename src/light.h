#ifndef LIGHT_H
#define LIGHT_H

#include <stdint.h>
#include "vector.h"

typedef struct {
	float intensity;
	vec3_t direction;
} light_t;

void init_light(float intensity, vec3_t direction);
float get_light_intensity(void);
vec3_t get_light_direction(void);
uint32_t apply_light_intensity(uint32_t original_color, float percentage_factor);

#endif
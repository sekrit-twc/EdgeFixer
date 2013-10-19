#ifndef EDGEFIXER_H
#define EDGEFIXER_H

#include <stdint.h>

typedef uint8_t pixel_t;

int process_edge(pixel_t *x, pixel_t *y, int dist_to_next, int n, int radius);

#endif

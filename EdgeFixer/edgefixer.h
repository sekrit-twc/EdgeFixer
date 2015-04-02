#ifndef EDGEFIXER_H
#define EDGEFIXER_H

#include <stddef.h>
#include <stdint.h>

size_t edgefixer_required_buffer(int n);

void edgefixer_process_edge(uint8_t *x, const uint8_t *y, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp);

#endif // EDGEFIXER_H

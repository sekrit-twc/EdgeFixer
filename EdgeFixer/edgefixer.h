#ifndef EDGEFIXER_H
#define EDGEFIXER_H

#include <stddef.h>
#include <stdint.h>

size_t edgefixer_required_buffer_b(int n);
size_t edgefixer_required_buffer_w(int n);

void edgefixer_process_edge_b(void *xptr, const void *yptr, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp);
void edgefixer_process_edge_w(void *xptr, const void *yptr, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp);

#endif /* EDGEFIXER_H */

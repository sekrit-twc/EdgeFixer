#include <math.h>
#include <stdlib.h>
#include "edgefixer.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct least_squares_data {
	uint64_t integral_x;
	uint64_t integral_y;
	uint64_t integral_xy;
	uint64_t integral_xsqr;
} least_squares_data;

static void least_squares(int n, least_squares_data *d, double *a, double *b)
{
	double interval_x = (double)(d[n].integral_x - d[0].integral_x);
	double interval_y = (double)(d[n].integral_y - d[0].integral_y);
	double interval_xy = (double)(d[n].integral_xy - d[0].integral_xy);
	double interval_xsqr = (double)(d[n].integral_xsqr - d[0].integral_xsqr);

	/* Add 0.001 to denominator to prevent division by zero. */
	*a = ((double)n * interval_xy - interval_x * interval_y) / ((interval_xsqr * (double)n - interval_x * interval_x) + 0.001);
	*b = (interval_y - *a * interval_x) / (double)n;
}

static uint8_t double_to_u8(double x, int depth)
{
	double maxval = (double)((1 << depth) - 1);
	return (uint8_t)lrint(MIN(MAX(x, 0), maxval));
}

static uint16_t double_to_u16(double x, int depth)
{
	double maxval = (double)((1 << depth) - 1);
	return (uint16_t)lrint(MIN(MAX(x, 0), maxval));
}

size_t edgefixer_required_buffer_b(int n)
{
	return ((size_t)n + 1) * sizeof(least_squares_data);
}

size_t edgefixer_required_buffer_w(int n)
{
	return ((size_t)n + 1) * sizeof(least_squares_data);
}

void edgefixer_process_edge_b(void *xptr, const void *yptr, ptrdiff_t x_dist_to_next, ptrdiff_t y_dist_to_next, int n, int radius, int depth, void *tmp)
{
	uint8_t *x = xptr;
	const uint8_t *y = yptr;

	least_squares_data *buf = (least_squares_data *)tmp;
	double a, b;
	int i;

	buf[0].integral_x = 0;
	buf[0].integral_y = 0;
	buf[0].integral_xy = 0;
	buf[0].integral_xsqr = 0;

	for (i = 0; i < n; ++i) {
		uint16_t _x = x[i * x_dist_to_next];
		uint16_t _y = y[i * y_dist_to_next];

		buf[i + 1].integral_x = buf[i].integral_x + _x;
		buf[i + 1].integral_y = buf[i].integral_y + _y;
		buf[i + 1].integral_xy = buf[i].integral_xy + _x * _y;
		buf[i + 1].integral_xsqr = buf[i].integral_xsqr + _x * _x;
	}

	if (radius) {
		for (i = 0; i < n; ++i) {
			int left = i - radius;
			int right = i + radius;

			if (left < 0)
				left = 0;
			if (right > n - 1)
				right = n - 1;
			least_squares(right - left + 1, buf + left, &a, &b);
			x[i * x_dist_to_next] = double_to_u8(x[i * x_dist_to_next] * a + b, depth);
		}
	} else {
		least_squares(n, buf, &a, &b);
		for (i = 0; i < n; ++i) {
			x[i * x_dist_to_next] = double_to_u8(x[i * x_dist_to_next] * a + b, depth);
		}
	}
}

void edgefixer_process_edge_w(void *xptr, const void *yptr, ptrdiff_t x_dist_to_next, ptrdiff_t y_dist_to_next, int n, int radius, int depth, void *tmp)
{
	uint16_t *x = xptr;
	const uint16_t *y = yptr;

	least_squares_data *buf = (least_squares_data *)tmp;
	double a, b;
	int i;

	x_dist_to_next /= 2;
	y_dist_to_next /= 2;

	buf[0].integral_x = 0;
	buf[0].integral_y = 0;
	buf[0].integral_xy = 0;
	buf[0].integral_xsqr = 0;

	for (i = 0; i < n; ++i) {
		uint32_t _x = x[i * x_dist_to_next];
		uint32_t _y = y[i * y_dist_to_next];

		buf[i + 1].integral_x = buf[i].integral_x + _x;
		buf[i + 1].integral_y = buf[i].integral_y + _y;
		buf[i + 1].integral_xy = buf[i].integral_xy + _x * _y;
		buf[i + 1].integral_xsqr = buf[i].integral_xsqr + _x * _x;
	}

	if (radius) {
		for (i = 0; i < n; ++i) {
			int left = i - radius;
			int right = i + radius;

			if (left < 0)
				left = 0;
			if (right > n - 1)
				right = n - 1;
			least_squares(right - left + 1, buf + left, &a, &b);
			x[i * x_dist_to_next] = double_to_u16(x[i * x_dist_to_next] * a + b, depth);
		}
	} else {
		least_squares(n, buf, &a, &b);
		for (i = 0; i < n; ++i) {
			x[i * x_dist_to_next] = double_to_u16(x[i * x_dist_to_next] * a + b, depth);
		}
	}
}

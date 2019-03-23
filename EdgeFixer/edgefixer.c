#include <math.h>
#include <stdlib.h>
#include "edgefixer.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct least_squares_data {
	int32_t integral_x;
	int32_t integral_y;
	int32_t integral_xy;
	int32_t integral_xsqr;
} least_squares_data;

typedef struct least_squares_data64 {
	int64_t integral_x;
	int64_t integral_y;
	int64_t integral_xy;
	int64_t integral_xsqr;
} least_squares_data64;

static void least_squares(int n, least_squares_data *d, float *a, float *b)
{
	float interval_x = (float)(d[n - 1].integral_x - d[0].integral_x);
	float interval_y = (float)(d[n - 1].integral_y - d[0].integral_y);
	float interval_xy = (float)(d[n - 1].integral_xy - d[0].integral_xy);
	float interval_xsqr = (float)(d[n - 1].integral_xsqr - d[0].integral_xsqr);

	/* Add 0.001f to denominator to prevent division by zero. */
	*a = ((float)n * interval_xy - interval_x * interval_y) / ((interval_xsqr * (float)n - interval_x * interval_x) + 0.001f);
	*b = (interval_y - *a * interval_x) / (float)n;
}

static void least_squares64(int n, least_squares_data64 *d, double *a, double *b)
{
	double interval_x = (double)(d[n - 1].integral_x - d[0].integral_x);
	double interval_y = (double)(d[n - 1].integral_y - d[0].integral_y);
	double interval_xy = (double)(d[n - 1].integral_xy - d[0].integral_xy);
	double interval_xsqr = (double)(d[n - 1].integral_xsqr - d[0].integral_xsqr);

	/* Add 0.001f to denominator to prevent division by zero. */
	*a = ((double)n * interval_xy - interval_x * interval_y) / ((interval_xsqr * (double)n - interval_x * interval_x) + 0.001f);
	*b = (interval_y - *a * interval_x) / (double)n;
}

static uint8_t float_to_u8(float x)
{
	return (uint8_t)lrintf(MIN(MAX(x, 0), UINT8_MAX));
}

static uint16_t double_to_u16(double x)
{
	return (uint16_t)lrint(MIN(MAX(x, 0), UINT16_MAX));
}

size_t edgefixer_required_buffer_b(int n)
{
	return n * sizeof(least_squares_data);
}

size_t edgefixer_required_buffer_w(int n)
{
	return n * sizeof(least_squares_data64);
}

void edgefixer_process_edge_b(void *xptr, const void *yptr, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp)
{
	uint8_t *x = xptr;
	const uint8_t *y = yptr;

	least_squares_data *buf = (least_squares_data *)tmp;
	float a, b;
	int i;

	buf[0].integral_x = x[0];
	buf[0].integral_y = y[0];
	buf[0].integral_xy = x[0] * y[0];
	buf[0].integral_xsqr = x[0] * x[0];

	for (i = 1; i < n; ++i) {
		uint16_t _x = x[i * x_dist_to_next / sizeof(uint8_t)];
		uint16_t _y = y[i * y_dist_to_next / sizeof(uint8_t)];

		buf[i].integral_x = buf[i - 1].integral_x + _x;
		buf[i].integral_y = buf[i - 1].integral_y + _y;
		buf[i].integral_xy = buf[i - 1].integral_xy + _x * _y;
		buf[i].integral_xsqr = buf[i - 1].integral_xsqr + _x * _x;
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
			x[i * x_dist_to_next / sizeof(uint8_t)] = float_to_u8(x[i * x_dist_to_next / sizeof(uint8_t)] * a + b);
		}
	} else {
		least_squares(n, buf, &a, &b);
		for (i = 0; i < n; ++i) {
			x[i * x_dist_to_next / sizeof(uint8_t)] = float_to_u8(x[i * x_dist_to_next / sizeof(uint8_t)] * a + b);
		}
	}
}

void edgefixer_process_edge_w(void *xptr, const void *yptr, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp)
{
	uint16_t *x = xptr;
	const uint16_t *y = yptr;

	least_squares_data64 *buf = (least_squares_data64 *)tmp;
	double a, b;
	int i;

	buf[0].integral_x = x[0];
	buf[0].integral_y = y[0];
	buf[0].integral_xy = (long long)x[0] * y[0];
	buf[0].integral_xsqr = (long long)x[0] * x[0];

	for (i = 1; i < n; ++i) {
		uint32_t _x = x[i * x_dist_to_next / sizeof(uint16_t)];
		uint32_t _y = y[i * y_dist_to_next / sizeof(uint16_t)];

		buf[i].integral_x = buf[i - 1].integral_x + _x;
		buf[i].integral_y = buf[i - 1].integral_y + _y;
		buf[i].integral_xy = buf[i - 1].integral_xy + _x * _y;
		buf[i].integral_xsqr = buf[i - 1].integral_xsqr + _x * _x;
	}

	if (radius) {
		for (i = 0; i < n; ++i) {
			int left = i - radius;
			int right = i + radius;

			if (left < 0)
				left = 0;
			if (right > n - 1)
				right = n - 1;
			least_squares64(right - left + 1, buf + left, &a, &b);
			x[i * x_dist_to_next / sizeof(uint16_t)] = double_to_u16(x[i * x_dist_to_next / sizeof(uint16_t)] * a + b);
		}
	} else {
		least_squares64(n, buf, &a, &b);
		for (i = 0; i < n; ++i) {
			x[i * x_dist_to_next / sizeof(uint16_t)] = double_to_u16(x[i * x_dist_to_next / sizeof(uint16_t)] * a + b);
		}
	}
}

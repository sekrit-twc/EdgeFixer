#include <math.h>
#include <stdlib.h>
#include "edgefixer.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct least_squares_data {
	int integral_x;
	int integral_y;
	int integral_xy;
	int integral_xsqr;
} least_squares_data;

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

static uint8_t float_to_u8(float x)
{
	return (uint8_t)lrintf(MIN(MAX(x, 0), UINT8_MAX));
}

size_t edgefixer_required_buffer(int n)
{
	return n * sizeof(least_squares_data);
}

void edgefixer_process_edge(uint8_t *x, const uint8_t *y, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp)
{
	least_squares_data *buf = (least_squares_data *)tmp;
	float a, b;
	int i;

	buf[0].integral_x = x[0];
	buf[0].integral_y = y[0];
	buf[0].integral_xy = x[0] * y[0];
	buf[0].integral_xsqr = x[0] * x[0];

	for (i = 1; i < n; ++i) {
		int _x = x[i * x_dist_to_next];
		int _y = y[i * y_dist_to_next];

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
			x[i * x_dist_to_next] = float_to_u8(x[i * x_dist_to_next] * a + b);
		}
	} else {
		least_squares(n, buf, &a, &b);
		for (i = 0; i < n; ++i) {
			x[i * x_dist_to_next] = float_to_u8(x[i * x_dist_to_next] * a + b);
		}
	}
}

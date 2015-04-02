#include <stdlib.h>
#include "EdgeFixer.h"

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
	float interval_x = (float) (d[n - 1].integral_x - d[0].integral_x);
	float interval_y = (float) (d[n - 1].integral_y - d[0].integral_y);
	float interval_xy = (float) (d[n - 1].integral_xy - d[0].integral_xy);
	float interval_xsqr = (float) (d[n - 1].integral_xsqr - d[0].integral_xsqr);
	
	// add 0.001f to denominator to prevent division by zero
	*a = ((float) n * interval_xy - interval_x * interval_y) / ((interval_xsqr * (float) n - interval_x * interval_x) + 0.001f);
	*b = (interval_y - *a * interval_x) / (float) n;
}

static uint8_t float_to_u8(float x)
{
	return (uint8_t)(MIN(MAX(x, 0), UINT8_MAX) + 0.5f);
}

size_t edgefixer_required_buffer(int n)
{
	return n * sizeof(least_squares_data);
}

void edgefixer_process_edge(uint8_t *x, const uint8_t *y, int x_dist_to_next, int y_dist_to_next, int n, int radius, void *tmp)
{
	least_squares_data *buf = (least_squares_data *) tmp;
	float a, b;

	buf[0].integral_x = x[0];
	buf[0].integral_y = y[0];
	buf[0].integral_xy = x[0] * y[0];
	buf[0].integral_xsqr = x[0] * x[0];

	for (int i = 1; i < n; ++i) {
		int _x = x[i * x_dist_to_next];
		int _y = y[i * y_dist_to_next];

		buf[i].integral_x = buf[i - 1].integral_x + _x;
		buf[i].integral_y = buf[i - 1].integral_y + _y;
		buf[i].integral_xy = buf[i - 1].integral_xy + _x * _y;
		buf[i].integral_xsqr = buf[i - 1].integral_xsqr + _x * _x;
	}

	if (radius) {
		for (int i = 0; i < n; ++i) {
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
		for (int i = 0; i < n; ++i)
			x[i * x_dist_to_next] = float_to_u8(x[i * x_dist_to_next] * a + b);
	}
}

#if 0
#include <stdio.h>

static int read_frame(pixel_t *buf, FILE *in, int width, int height)
{
	for (int i = 0; i < height; ++i) {
		if (fread(buf + i * width, sizeof(pixel_t), width, in) != width)
			return -1;
	}
	return 0;
}

static int write_frame(pixel_t *buf, FILE *out, int width, int height)
{
	for (int i = 0; i < height; ++i) {
		if (fwrite(buf + i * width, sizeof(pixel_t), width, out) != width)
			return -1;
	}
	return 0;
}

static int fix_edges(const char *infile, const char *outfile, int width, int height, int radius)
{
	FILE *in = NULL, *out = NULL;
	pixel_t *frame = NULL;
	void *tmp = NULL;
	int ret = -1;

	in = fopen(infile, "rb");
	if (!in) {
		fprintf(stderr, "error opening input file: %s\n", infile);
		goto fail;
	}

	out = fopen(outfile, "wb");
	if (!out) {
		fprintf(stderr, "error opening output file: %s\n", outfile);
		goto fail;
	}

	frame = (pixel_t *) malloc(width * height * sizeof(pixel_t));
	if (!frame) {
		fprintf(stderr, "error allocating frame\n");
		goto fail;
	}

	if (read_frame(frame, in, width, height)) {
		fprintf(stderr, "error reading frame");
		goto fail;
	}

	tmp = malloc(required_buffer(width > height ? width : height));
	if (!tmp) {
		fprintf(stderr, "error allocating temporary buffer");
		goto fail;
	}

	// left
	process_edge(&frame[0], &frame[1], width, width, height, radius, tmp);

	// right
	process_edge(&frame[width - 1], &frame[width - 2], width, width, height, radius, tmp);

	// top
	process_edge(&frame[0], &frame[width], 1, 1, width, radius, tmp);

	// bottom
	process_edge(&frame[width * (height - 1)], &frame[width * (height - 2)], 1, 1, width, radius, tmp);

	if (write_frame(frame, out, width, height)) {
		fprintf(stderr, "error writing frame");
		goto fail;
	}

	ret = 0;
fail:
	free(tmp);
	free(frame);
	if (out)
		fclose(out);
	if (in)
		fclose(in);
	return ret;
}

int main(int argc, const char **argv)
{
	int width, height, radius;
	const char *infile, *outfile;

	if (argc != 6) {
		fprintf(stderr, "Usage: EdgeFixer width height radius infile outfile\n");
		return -1;
	}

	width = atoi(argv[1]);
	if (width == 0) {
		fprintf(stderr, "error in width\n");
		return -1;
	}

	height = atoi(argv[2]);
	if (height == 0) {
		fprintf(stderr, "error in height\n");
		return -1;
	}

	radius = atoi(argv[3]);
	infile = argv[4];
	outfile = argv[5];

	if (fix_edges(infile, outfile, width, height, radius)) {
		fprintf(stderr, "internal error\n");
		return -1;
	}

	return 0;
}
#endif

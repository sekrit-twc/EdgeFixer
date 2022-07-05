#include <VapourSynth4.h>
#include <VSHelper4.h>
#include "edgefixer.h"

typedef struct vs_edgefix_data {
	VSNode *node;
	VSNode *ref_node;
	VSVideoInfo vi;
	int left;
	int top;
	int right;
	int bottom;
	int radius;
} vs_edgefix_data;

static const VSFrame * VS_CC vs_continuity_get_frame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = instanceData;
	VSFrame *ret = 0;
	int i;

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, data->node, frameCtx);
	} else if (activationReason == arAllFramesReady) {
		const VSFrame *src_frame = vsapi->getFrameFilter(n, data->node, frameCtx);
		const VSFrame *src_planes[3] = { src_frame, src_frame, src_frame };
		const VSVideoFormat *format = vsapi->getVideoFrameFormat(src_frame);
		int plane_order[3] = { 0, 1, 2 };

		int width = vsapi->getFrameWidth(src_frame, 0);
		int height = vsapi->getFrameHeight(src_frame, 0);

		size_t (*required_buffer)(int) = format->bytesPerSample == 2 ? edgefixer_required_buffer_w : edgefixer_required_buffer_b;
		void (*process_edge)(void *, const void *, ptrdiff_t, ptrdiff_t, int, int, void *) = format->bytesPerSample == 2 ? edgefixer_process_edge_w : edgefixer_process_edge_b;

		VSFrame *dst_frame = vsapi->newVideoFrame2(format, width, height, src_planes, plane_order, src_frame, core);
		uint8_t *ptr = vsapi->getWritePtr(dst_frame, 0);
		ptrdiff_t stride = vsapi->getStride(dst_frame, 0);
		int step = format->bytesPerSample;

		void *tmp = malloc(required_buffer(width > height ? width : height));
		if (!tmp) {
			vsapi->setFilterError("error allocating buffer", frameCtx);
			goto fail;
		}

		for (i = 0; i < data->top; ++i) {
			int ref_row = data->top - i;
			process_edge(ptr + stride * (ref_row - 1), ptr + stride * ref_row, step, step, width, data->radius, tmp);
		}
		for (i = 0; i < data->bottom; ++i) {
			int ref_row = height - data->bottom - 1 + i;
			process_edge(ptr + stride * (ref_row + 1), ptr + stride * ref_row, step, step, width, data->radius, tmp);
		}
		for (i = 0; i < data->left; ++i) {
			int ref_col = data->left - i;
			process_edge(ptr + step * (ref_col - 1), ptr + step * ref_col, stride, stride, height, data->radius, tmp);
		}
		for (i = 0; i < data->right; ++i) {
			int ref_col = width - data->right - 1 + i;
			process_edge(ptr + step * (ref_col + 1), ptr + step * ref_col, stride, stride, height, data->radius, tmp);
		}

		ret = dst_frame;
		dst_frame = 0;
	fail:
		vsapi->freeFrame(src_frame);
		vsapi->freeFrame(dst_frame);
		free(tmp);
	}

	return ret;
}

static const VSFrame * VS_CC vs_reference_get_frame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = instanceData;
	VSFrame *ret = 0;
	int i;

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, data->node, frameCtx);
		vsapi->requestFrameFilter(n, data->ref_node, frameCtx);
	} else if (activationReason == arAllFramesReady) {
		const VSFrame *src_frame = vsapi->getFrameFilter(n, data->node, frameCtx);
		const VSFrame *src_planes[3] = { src_frame, src_frame, src_frame };
		const VSVideoFormat *format = vsapi->getVideoFrameFormat(src_frame);
		int plane_order[3] = { 0, 1, 2 };

		int width = vsapi->getFrameWidth(src_frame, 0);
		int height = vsapi->getFrameHeight(src_frame, 0);

		size_t (*required_buffer)(int) = format->bytesPerSample == 2 ? edgefixer_required_buffer_w : edgefixer_required_buffer_b;
		void (*process_edge)(void *, const void *, ptrdiff_t, ptrdiff_t, int, int, void *) = format->bytesPerSample == 2 ? edgefixer_process_edge_w : edgefixer_process_edge_b;

		VSFrame *dst_frame = vsapi->newVideoFrame2(format, width, height, src_planes, plane_order, src_frame, core);
		uint8_t *ptr = vsapi->getWritePtr(dst_frame, 0);
		ptrdiff_t stride = vsapi->getStride(dst_frame, 0);

		const VSFrame *ref_frame = vsapi->getFrameFilter(n, data->ref_node, frameCtx);
		const uint8_t *ref_ptr = vsapi->getReadPtr(ref_frame, 0);
		ptrdiff_t ref_stride = vsapi->getStride(ref_frame, 0);
		int step = format->bytesPerSample;

		void *tmp = malloc(required_buffer(width > height ? width : height));
		if (!tmp) {
			vsapi->setFilterError("error allocating buffer", frameCtx);
			goto fail;
		}

		for (i = 0; i < data->top; ++i) {
			process_edge(ptr + stride * i, ref_ptr + ref_stride * i, step, step, width, data->radius, tmp);
		}
		for (i = 0; i < data->bottom; ++i) {
			process_edge(ptr + stride * (data->vi.height - i - 1), ref_ptr + ref_stride * (height - i - 1), step, step, width, data->radius, tmp);
		}
		for (i = 0; i < data->left; ++i) {
			process_edge(ptr + step * i, ref_ptr + step * i, stride, ref_stride, height, data->radius, tmp);
		}
		for (i = 0; i < data->right; ++i) {
			process_edge(ptr + step * (width - i - 1), ref_ptr + step * (width - i - 1), stride, ref_stride, height, data->radius, tmp);
		}

		ret = dst_frame;
		dst_frame = 0;
	fail:
		vsapi->freeFrame(src_frame);
		vsapi->freeFrame(dst_frame);
		vsapi->freeFrame(ref_frame);
		free(tmp);
	}

	return ret;
}

static void VS_CC vs_edgefix_free(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = instanceData;

	vsapi->freeNode(data->node);
	vsapi->freeNode(data->ref_node);
	free(data);
}

static void VS_CC vs_edgefix_create(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = 0;
	VSNode *node = 0;
	VSNode *ref_node = 0;
	VSVideoInfo vi;
	int left, top, right, bottom, radius;
	int err;

	node = vsapi->mapGetNode(in, "clip", 0, 0);
	if ((intptr_t)userData)
		ref_node = vsapi->mapGetNode(in, "ref", 0, 0);

	vi = *vsapi->getVideoInfo(node);

	left = (int)vsapi->mapGetInt(in, "left", 0, &err);
	if (err)
		left = 0;

	top = (int)vsapi->mapGetInt(in, "top", 0, &err);
	if (err)
		top = 0;

	right = (int)vsapi->mapGetInt(in, "right", 0, &err);
	if (err)
		right = 0;

	bottom = (int)vsapi->mapGetInt(in, "bottom", 0, &err);
	if (err)
		bottom = 0;

	radius = (int)vsapi->mapGetInt(in, "radius", 0, &err);
	if (err)
		radius = 0;

	if (vi.format.colorFamily == cfRGB) {
		vsapi->mapSetError(out, "only YUV is supported");
		goto fail;
	}
	if (vi.format.bytesPerSample > 2 || vi.format.sampleType != stInteger) {
		vsapi->mapSetError(out, "only BYTE and WORD are supported");
		goto fail;
	}
	if (ref_node && !vsh_isSameVideoFormat(&vi.format, &vsapi->getVideoInfo(ref_node)->format)) {
		vsapi->mapSetError(out, "clip and reference must have same format");
		goto fail;
	}

	if (left < 0 || right < 0 || top < 0 || bottom < 0) {
		vsapi->mapSetError(out, "too few edges to fix");
		goto fail;
	}
	if (left > vi.width || right > vi.width || top > vi.height || bottom > vi.height) {
		vsapi->mapSetError(out, "too many edges to fix");
		goto fail;
	}

	data = malloc(sizeof(vs_edgefix_data));
	if (!data) {
		vsapi->mapSetError(out, "error allocating data");
		goto fail;
	}

	data->node = node;
	data->ref_node = ref_node;
	data->vi = vi;
	data->left = left;
	data->top = top;
	data->right = right;
	data->bottom = bottom;
	data->radius = radius;

	{
		VSFilterDependency deps[] = { { node, rpStrictSpatial }, { ref_node, rpStrictSpatial } };
		VSFilterGetFrame get_frame = ref_node ? vs_reference_get_frame : vs_continuity_get_frame;
		int ndeps = ref_node ? 2 : 1;
		vsapi->createVideoFilter(out, "edgefixer", &vi, get_frame, vs_edgefix_free, fmParallel, deps, ndeps, data, core);
	}
	return;
fail:
	vsapi->freeNode(ref_node);
	vsapi->freeNode(node);
	free(data);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, VSPLUGINAPI *vspapi)
{
	vspapi->configPlugin("the.weather.channel", "edgefixer", "ultraman", 0, VAPOURSYNTH_API_VERSION, 1, plugin);

#define COMMON_ARGS "left:int:opt;top:int:opt;right:int:opt;bottom:int:opt;radius:int:opt;"
	vspapi->registerFunction("Continuity", "clip:vnode;" COMMON_ARGS, "clip:vnode;", vs_edgefix_create, (void *)0, plugin);
	vspapi->registerFunction("Reference", "clip:clip;ref:clip;" COMMON_ARGS, "clip:vnode;", vs_edgefix_create, (void *)1, plugin);
#undef COMMON_ARGS
}

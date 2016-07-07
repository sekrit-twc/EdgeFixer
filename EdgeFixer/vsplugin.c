#include "edgefixer.h"
#include "VapourSynth.h"
#include "VSHelper.h"

typedef struct vs_edgefix_data {
	VSNodeRef *node;
	VSNodeRef *ref_node;
	VSVideoInfo vi;
	int left;
	int top;
	int right;
	int bottom;
	int radius;
} vs_edgefix_data;

static void VS_CC vs_edgefix_init(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
	const vs_edgefix_data *data = *instanceData;
	vsapi->setVideoInfo(&data->vi, 1, node);
}

static const VSFrameRef * VS_CC vs_continuity_get_frame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = *instanceData;
	VSFrameRef *ret = 0;
	int i;

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, data->node, frameCtx);
	} else if (activationReason == arAllFramesReady) {
		const VSFrameRef *src_frame = vsapi->getFrameFilter(n, data->node, frameCtx);
		const VSFrameRef *src_planes[3] = { src_frame, src_frame, src_frame };
		int plane_order[3] = { 0, 1, 2 };

		VSFrameRef *dst_frame = vsapi->newVideoFrame2(data->vi.format, data->vi.width, data->vi.height, src_planes, plane_order, src_frame, core);
		uint8_t *ptr = vsapi->getWritePtr(dst_frame, 0);
		int stride = vsapi->getStride(dst_frame, 0);

		void *tmp = malloc(edgefixer_required_buffer(data->vi.width > data->vi.height ? data->vi.width : data->vi.height));
		if (!tmp) {
			vsapi->setFilterError("error allocating buffer", frameCtx);
			goto fail;
		}
		
		for (i = 0; i < data->top; ++i) {
			int ref_row = data->top - i;
			edgefixer_process_edge(ptr + stride * (ref_row - 1), ptr + stride * ref_row, 1, 1, data->vi.width, data->radius, tmp);
		}
		for (i = 0; i < data->bottom; ++i) {
			int ref_row = data->vi.height - data->bottom - 1 + i;
			edgefixer_process_edge(ptr + stride * (ref_row + 1), ptr + stride * ref_row, 1, 1, data->vi.width, data->radius, tmp);
		}
		for (i = 0; i < data->left; ++i) {
			int ref_col = data->left - i;
			edgefixer_process_edge(ptr + ref_col - 1, ptr + ref_col, stride, stride, data->vi.height, data->radius, tmp);
		}
		for (i = 0; i < data->radius; ++i) {
			int ref_col = data->vi.width - data->right - 1 + i;
			edgefixer_process_edge(ptr + ref_col + 1, ptr + ref_col, stride, stride, data->vi.height, data->radius, tmp);
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

static const VSFrameRef * VS_CC vs_reference_get_frame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
	vs_edgefix_data *data = *instanceData;
	VSFrameRef *ret = 0;
	int i;

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, data->node, frameCtx);
		vsapi->requestFrameFilter(n, data->ref_node, frameCtx);
	} else if (activationReason == arAllFramesReady) {
		const VSFrameRef *src_frame = vsapi->getFrameFilter(n, data->node, frameCtx);
		const VSFrameRef *src_planes[3] = { src_frame, src_frame, src_frame };
		int plane_order[3] = { 0, 1, 2 };

		VSFrameRef *dst_frame = vsapi->newVideoFrame2(data->vi.format, data->vi.width, data->vi.height, src_planes, plane_order, src_frame, core);
		uint8_t *ptr = vsapi->getWritePtr(dst_frame, 0);
		int stride = vsapi->getStride(dst_frame, 0);

		const VSFrameRef *ref_frame = vsapi->getFrameFilter(n, data->ref_node, frameCtx);
		const uint8_t *ref_ptr = vsapi->getReadPtr(ref_frame, 0);
		int ref_stride = vsapi->getStride(ref_frame, 0);

		void *tmp = malloc(edgefixer_required_buffer(data->vi.width > data->vi.height ? data->vi.width : data->vi.height));
		if (!tmp) {
			vsapi->setFilterError("error allocating buffer", frameCtx);
			goto fail;
		}

		for (i = 0; i < data->top; ++i) {
			edgefixer_process_edge(ptr + stride * i, ref_ptr + ref_stride * i, 1, 1, data->vi.width, data->radius, tmp);
		}
		for (i = 0; i < data->bottom; ++i) {
			edgefixer_process_edge(ptr + stride * (data->vi.height - i - 1), ref_ptr + ref_stride * (data->vi.height - i - 1), 1, 1, data->vi.width, data->radius, tmp);
		}
		for (i = 0; i < data->left; ++i) {
			edgefixer_process_edge(ptr + i, ref_ptr + i, stride, ref_stride, data->vi.height, data->radius, tmp);
		}
		for (i = 0; i < data->radius; ++i) {
			edgefixer_process_edge(ptr + data->vi.width - i - 1, ref_ptr + data->vi.width - i - 1, stride, ref_stride, data->vi.height, data->radius, tmp);
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
	VSNodeRef *node = 0;
	VSNodeRef *ref_node = 0;
	VSVideoInfo vi;
	int left, top, right, bottom, radius;
	int err;

	node = vsapi->propGetNode(in, "clip", 0, 0);
	if ((int)userData)
		ref_node = vsapi->propGetNode(in, "ref", 0, 0);

	vi = *vsapi->getVideoInfo(node);

	left = (int)vsapi->propGetInt(in, "left", 0, &err);
	if (err)
		left = 0;

	top = (int)vsapi->propGetInt(in, "top", 0, &err);
	if (err)
		top = 0;

	right = (int)vsapi->propGetInt(in, "right", 0, &err);
	if (err)
		right = 0;

	bottom = (int)vsapi->propGetInt(in, "bottom", 0, &err);
	if (err)
		bottom = 0;

	radius = (int)vsapi->propGetInt(in, "radius", 0, &err);
	if (err)
		radius = 0;

	if (vi.format->colorFamily == cmRGB) {
		vsapi->setError(out, "only YUV is supported");
		goto fail;
	}
	if (vi.format->bytesPerSample != 1 || vi.format->sampleType != stInteger) {
		vsapi->setError(out, "only BYTE is supported");
		goto fail;
	}
	if (ref_node && !isSameFormat(&vi, vsapi->getVideoInfo(ref_node))) {
		vsapi->setError(out, "clip and reference must have same format");
		goto fail;
	}

	if (left < 0 || right < 0 || top < 0 || bottom < 0) {
		vsapi->setError(out, "too few edges to fix");
		goto fail;
	}
	if (left > vi.width || right > vi.width || top > vi.height || bottom > vi.height) {
		vsapi->setError(out, "too many edges to fix");
		goto fail;
	}

	data = malloc(sizeof(vs_edgefix_data));
	if (!data) {
		vsapi->setError(out, "error allocating data");
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

	vsapi->createFilter(in, out, "edgefixer", vs_edgefix_init, ref_node ? vs_continuity_get_frame : vs_reference_get_frame, vs_edgefix_free, fmParallel, 0, data, core);

	return;
fail:
	free(data);
	return;
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
	configFunc("the.weather.channel", "edgefixer", "ultraman", VAPOURSYNTH_API_VERSION, 1, plugin);

	registerFunc("Continuity", "clip:clip;left:int:opt;top:int:opt;right:int:opt;bottom:int:opt;radius:int:opt;", vs_edgefix_create, (void *)0, plugin);
	registerFunc("Reference", "clip:clip;ref:clip;left:int:opt;top:int:opt;right:int:opt;bottom:int:opt;radius:int:opt;", vs_edgefix_create, (void *)1, plugin);
}

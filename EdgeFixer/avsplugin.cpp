#include <Windows.h>
#include "avisynth.h"

extern "C" {
#include "EdgeFixer.h"
}

class EdgeFixer : public GenericVideoFilter
{
	int m_radius;
public:
	EdgeFixer(PClip _child, int radius, IScriptEnvironment *env);

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env);
};

EdgeFixer::EdgeFixer(PClip _child, int radius, IScriptEnvironment *env)
	: GenericVideoFilter(_child), m_radius(radius)
{}

PVideoFrame __stdcall EdgeFixer::GetFrame(int n, IScriptEnvironment *env)
{
	PVideoFrame frame;
	int width, height, stride;
	BYTE *write_ptr;

	frame = child->GetFrame(n, env);
	env->MakeWritable(&frame);
	write_ptr = frame->GetWritePtr();

	width = frame->GetRowSize();
	height = frame->GetHeight();
	stride = frame->GetPitch();

	// top
	if (process_edge(write_ptr, write_ptr + stride, 1, width, m_radius))
		throw AvisynthError("[EdgeFixer] internal error");
	// bottom
	if (process_edge(write_ptr + stride * (height - 1), write_ptr + stride * (height - 2), 1, width, m_radius))
		throw AvisynthError("[EdgeFixer] internal error");
	// left
	if (process_edge(write_ptr, write_ptr + 1, stride, height, m_radius))
		throw AvisynthError("[EdgeFixer] internal error");
	// right
	if (process_edge(write_ptr + width - 1, write_ptr + width - 2, stride, height, m_radius))
		throw AvisynthError("[EdgeFixer] internal error");

	return frame;
}

AVSValue __cdecl Create_EdgeFixer(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	if (!args[0].AsClip()->GetVideoInfo().IsPlanar())
		throw AvisynthError("input clip must be planar");

	return new EdgeFixer(args[0].AsClip(), args[1].AsInt(0), env);
}

extern "C" __declspec(dllexport)
const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("EdgeFixer", "c[radius]i", Create_EdgeFixer, NULL);
	return "EdgeFixer";
}

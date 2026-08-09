// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GSMTLShaderCommon.h"

constant uint FMT_32 = 0;
constant uint FMT_24 = 1;
constant uint FMT_16 = 2;

constant uint SHUFFLE_READ = 1;
[[maybe_unused]] constant uint SHUFFLE_WRITE = 2;
constant uint SHUFFLE_READWRITE = 3;

constant bool HAS_FBFETCH           [[function_constant(GSMTLConstantIndex_FRAMEBUFFER_FETCH)]];
constant bool DEPTH_FEEDBACK        [[function_constant(GSMTLConstantIndex_DEPTH_FEEDBACK)]];
constant bool ROV_NEEDS_R32         [[function_constant(GSMTLConstantIndex_ROV_NEEDS_R32)]];
constant bool FST                   [[function_constant(GSMTLConstantIndex_FST)]];
constant bool IIP                   [[function_constant(GSMTLConstantIndex_IIP)]];
constant bool VS_POINT_SIZE         [[function_constant(GSMTLConstantIndex_VS_POINT_SIZE)]];
constant uint VS_EXPAND_TYPE_RAW    [[function_constant(GSMTLConstantIndex_VS_EXPAND_TYPE)]];
constant uint PS_AEM_FMT            [[function_constant(GSMTLConstantIndex_PS_AEM_FMT)]];
constant uint PS_PAL_FMT            [[function_constant(GSMTLConstantIndex_PS_PAL_FMT)]];
constant uint PS_DST_FMT            [[function_constant(GSMTLConstantIndex_PS_DST_FMT)]];
constant uint PS_DEPTH_FMT          [[function_constant(GSMTLConstantIndex_PS_DEPTH_FMT)]];
constant bool PS_AEM                [[function_constant(GSMTLConstantIndex_PS_AEM)]];
constant bool PS_FBA                [[function_constant(GSMTLConstantIndex_PS_FBA)]];
constant bool PS_FOG                [[function_constant(GSMTLConstantIndex_PS_FOG)]];
constant uint PS_DATE               [[function_constant(GSMTLConstantIndex_PS_DATE)]];
constant uint PS_ATST_RAW           [[function_constant(GSMTLConstantIndex_PS_ATST)]];
constant uint PS_AFAIL_RAW          [[function_constant(GSMTLConstantIndex_PS_AFAIL)]];
constant uint PS_ZTST_RAW           [[function_constant(GSMTLConstantIndex_PS_ZTST)]];
constant uint PS_TFX                [[function_constant(GSMTLConstantIndex_PS_TFX)]];
constant bool PS_TCC                [[function_constant(GSMTLConstantIndex_PS_TCC)]];
constant uint PS_WMS                [[function_constant(GSMTLConstantIndex_PS_WMS)]];
constant uint PS_WMT                [[function_constant(GSMTLConstantIndex_PS_WMT)]];
constant bool PS_ADJS               [[function_constant(GSMTLConstantIndex_PS_ADJS)]];
constant bool PS_ADJT               [[function_constant(GSMTLConstantIndex_PS_ADJT)]];
constant bool PS_LTF                [[function_constant(GSMTLConstantIndex_PS_LTF)]];
constant bool PS_SHUFFLE            [[function_constant(GSMTLConstantIndex_PS_SHUFFLE)]];
constant bool PS_SHUFFLE_SAME       [[function_constant(GSMTLConstantIndex_PS_SHUFFLE_SAME)]];
constant uint PS_PROCESS_BA         [[function_constant(GSMTLConstantIndex_PS_PROCESS_BA)]];
constant uint PS_PROCESS_RG         [[function_constant(GSMTLConstantIndex_PS_PROCESS_RG)]];
constant bool PS_SHUFFLE_ACROSS     [[function_constant(GSMTLConstantIndex_PS_SHUFFLE_ACROSS)]];
constant bool PS_READ16_SRC         [[function_constant(GSMTLConstantIndex_PS_READ16_SRC)]];
constant bool PS_WRITE_RG           [[function_constant(GSMTLConstantIndex_PS_WRITE_RG)]];
constant bool PS_FBMASK             [[function_constant(GSMTLConstantIndex_PS_FBMASK)]];
constant uint PS_BLEND_A            [[function_constant(GSMTLConstantIndex_PS_BLEND_A)]];
constant uint PS_BLEND_B            [[function_constant(GSMTLConstantIndex_PS_BLEND_B)]];
constant uint PS_BLEND_C            [[function_constant(GSMTLConstantIndex_PS_BLEND_C)]];
constant uint PS_BLEND_D            [[function_constant(GSMTLConstantIndex_PS_BLEND_D)]];
constant uint PS_BLEND_HW           [[function_constant(GSMTLConstantIndex_PS_BLEND_HW)]];
constant bool PS_A_MASKED           [[function_constant(GSMTLConstantIndex_PS_A_MASKED)]];
constant bool PS_COLCLIP_HW         [[function_constant(GSMTLConstantIndex_PS_COLCLIP_HW)]];
constant bool PS_RTA_CORRECTION     [[function_constant(GSMTLConstantIndex_PS_RTA_CORRECTION)]];
constant bool PS_RTA_SRC_CORRECTION [[function_constant(GSMTLConstantIndex_PS_RTA_SRC_CORRECTION)]];
constant bool PS_COLCLIP            [[function_constant(GSMTLConstantIndex_PS_COLCLIP)]];
constant uint PS_BLEND_MIX          [[function_constant(GSMTLConstantIndex_PS_BLEND_MIX)]];
constant bool PS_ROUND_INV          [[function_constant(GSMTLConstantIndex_PS_ROUND_INV)]];
constant bool PS_FIXED_ONE_A        [[function_constant(GSMTLConstantIndex_PS_FIXED_ONE_A)]];
constant bool PS_PABE               [[function_constant(GSMTLConstantIndex_PS_PABE)]];
constant bool PS_NO_COLOR           [[function_constant(GSMTLConstantIndex_PS_NO_COLOR)]];
constant bool PS_NO_COLOR1          [[function_constant(GSMTLConstantIndex_PS_NO_COLOR1)]];
constant uint PS_CHANNEL            [[function_constant(GSMTLConstantIndex_PS_CHANNEL)]];
constant uint PS_DITHER             [[function_constant(GSMTLConstantIndex_PS_DITHER)]];
constant uint PS_DITHER_ADJUST      [[function_constant(GSMTLConstantIndex_PS_DITHER_ADJUST)]];
constant bool PS_ZCLAMP             [[function_constant(GSMTLConstantIndex_PS_ZCLAMP)]];
constant bool PS_ZFLOOR             [[function_constant(GSMTLConstantIndex_PS_ZFLOOR)]];
constant bool PS_TCOFFSETHACK       [[function_constant(GSMTLConstantIndex_PS_TCOFFSETHACK)]];
constant bool PS_URBAN_CHAOS_HLE    [[function_constant(GSMTLConstantIndex_PS_URBAN_CHAOS_HLE)]];
constant bool PS_TALES_OF_ABYSS_HLE [[function_constant(GSMTLConstantIndex_PS_TALES_OF_ABYSS_HLE)]];
constant bool PS_TEX_IS_FB          [[function_constant(GSMTLConstantIndex_PS_TEX_IS_FB)]];
constant bool PS_AUTOMATIC_LOD      [[function_constant(GSMTLConstantIndex_PS_AUTOMATIC_LOD)]];
constant bool PS_MANUAL_LOD         [[function_constant(GSMTLConstantIndex_PS_MANUAL_LOD)]];
constant bool PS_REGION_RECT        [[function_constant(GSMTLConstantIndex_PS_REGION_RECT)]];
constant uint PS_SCANMSK            [[function_constant(GSMTLConstantIndex_PS_SCANMSK)]];
constant uint PS_AA1_RAW            [[function_constant(GSMTLConstantIndex_PS_AA1)]];
constant bool PS_ABE                [[function_constant(GSMTLConstantIndex_PS_ABE)]];
constant uint PS_SW_ANISO           [[function_constant(GSMTLConstantIndex_PS_SW_ANISO)]];
constant bool PS_ROV_COLOR          [[function_constant(GSMTLConstantIndex_PS_ROV_COLOR)]];
constant uint PS_ROV_DEPTH_RAW      [[function_constant(GSMTLConstantIndex_PS_ROV_DEPTH)]];

using GSShader::VSExpand;
using AFAIL = GSShader::PS_AFAIL;
using ATST = GSShader::PS_ATST;
using GSShader::ZTST;
using AA1 = GSShader::PS_AA1;
using ROV_DEPTH = GSShader::PS_ROV_DEPTH;
constant VSExpand VS_EXPAND_TYPE = static_cast<VSExpand>(VS_EXPAND_TYPE_RAW);
constant AFAIL PS_AFAIL = static_cast<AFAIL>(PS_AFAIL_RAW);
constant ATST  PS_ATST  = static_cast<ATST>(PS_ATST_RAW);
constant ZTST  PS_ZTST  = static_cast<ZTST>(PS_ZTST_RAW);
constant AA1   PS_AA1   = static_cast<AA1>(PS_AA1_RAW);
constant ROV_DEPTH PS_ROV_DEPTH = static_cast<ROV_DEPTH>(PS_ROV_DEPTH_RAW);

#if defined(__METAL_MACOS__) && __METAL_VERSION__ >= 220
	#define PRIMID_SUPPORT 1
#else
	#define PRIMID_SUPPORT 0
#endif

#if defined(__METAL_IOS__) || __METAL_VERSION__ >= 230
	#define FBFETCH_SUPPORT 1
#else
	#define FBFETCH_SUPPORT 0
#endif

constant bool PS_PRIM_CHECKING_INIT = PS_DATE == 1 || PS_DATE == 2;
constant bool PS_PRIM_CHECKING_READ = PS_DATE == 3;
#if PRIMID_SUPPORT
constant bool NEEDS_PRIMID = PS_PRIM_CHECKING_INIT || PS_PRIM_CHECKING_READ;
#endif
constant bool PS_TEX_IS_DEPTH = PS_URBAN_CHAOS_HLE || PS_TALES_OF_ABYSS_HLE || PS_DEPTH_FMT == 1 || PS_DEPTH_FMT == 2;
constant bool PS_TEX_IS_COLOR = !PS_TEX_IS_DEPTH;
constant bool PS_HAS_PALETTE = PS_PAL_FMT != 0 || (PS_CHANNEL >= 1 && PS_CHANNEL <= 5);
constant bool NOT_IIP = !IIP;
constant bool SW_BLEND = (PS_BLEND_A != PS_BLEND_B) || PS_BLEND_D;
constant bool SW_AD_TO_HW = (PS_BLEND_C == 1 && PS_A_MASKED);
constant bool NEEDS_RT_FOR_BLEND = (((PS_BLEND_A != PS_BLEND_B) && (PS_BLEND_A == 1 || PS_BLEND_B == 1 || PS_BLEND_C == 1)) || PS_BLEND_D == 1 || SW_AD_TO_HW);
constant bool NEEDS_RT_EARLY = PS_TEX_IS_FB || PS_DATE >= 5;
constant bool NEEDS_RT_FOR_AFAIL = PS_AFAIL == AFAIL::ZB_ONLY || PS_AFAIL == AFAIL::RGB_ONLY || PS_AFAIL == AFAIL::RGB_ONLY_SW_Z;
constant bool NEEDS_RT = NEEDS_RT_FOR_AFAIL || NEEDS_RT_EARLY || (!PS_PRIM_CHECKING_INIT && (PS_FBMASK || NEEDS_RT_FOR_BLEND));
constant bool NEEDS_DEPTH_FOR_AFAIL = PS_AFAIL == AFAIL::FB_ONLY || PS_AFAIL == AFAIL::RGB_ONLY_SW_Z;
constant bool NEEDS_DEPTH_FOR_ZTST  = PS_ZTST == ZTST::GEQUAL || PS_ZTST == ZTST::GREATER;
constant bool NEEDS_DEPTH_FOR_AA1   = PS_AA1 == AA1::TRIANGLE_SW_Z;
constant bool SW_DEPTH = NEEDS_DEPTH_FOR_AFAIL || NEEDS_DEPTH_FOR_ZTST || NEEDS_DEPTH_FOR_AA1;

constant bool PS_OUTPUT_COLOR0 = !PS_NO_COLOR  && !PS_ROV_COLOR;
constant bool PS_OUTPUT_COLOR1 = !PS_NO_COLOR1 && !PS_ROV_COLOR;
constant bool PS_ZOUTPUT = (PS_ZCLAMP || PS_ZFLOOR || SW_DEPTH) && PS_ROV_DEPTH == ROV_DEPTH::NONE;
constant bool PS_ZOUTPUT_LESS = PS_ZOUTPUT && !SW_DEPTH;
constant bool PS_ZOUTPUT_ANY  = PS_ZOUTPUT && SW_DEPTH;
constant bool PS_ZOUTPUT_COLOR = PS_ZOUTPUT_ANY && !DEPTH_FEEDBACK;
constant bool VS_NEEDS_INDEX_BUFFER = VS_EXPAND_TYPE == VSExpand::TriangleAA1;
constant bool VS_COVERAGE = VS_EXPAND_TYPE == VSExpand::LineAA1 || VS_EXPAND_TYPE == VSExpand::TriangleAA1;
constant bool VS_INTERIOR = VS_EXPAND_TYPE == VSExpand::TriangleAA1;
constant bool PS_COVERAGE = PS_AA1 != AA1::NONE;
constant bool PS_INTERIOR = PS_AA1 == AA1::TRIANGLE_SW_Z;

struct MainVSIn
{
	float2 st [[attribute(GSMTLAttributeIndexST)]];
	float4 c  [[attribute(GSMTLAttributeIndexC)]];
	float  q  [[attribute(GSMTLAttributeIndexQ)]];
	uint2  p  [[attribute(GSMTLAttributeIndexXY)]];
	uint   z  [[attribute(GSMTLAttributeIndexZ)]];
	uint2  uv [[attribute(GSMTLAttributeIndexUV)]];
	float4 f  [[attribute(GSMTLAttributeIndexF)]];
};

struct MainVSOut
{
	float4 p [[position]];
	float4 t;
	float4 ti;
	float4 c [[function_constant(IIP)]];
	float4 fc [[flat, function_constant(NOT_IIP)]];
	float inv_cov [[function_constant(VS_COVERAGE)]];
	uint interior [[function_constant(VS_INTERIOR)]];
	float point_size [[point_size, function_constant(VS_POINT_SIZE)]];
};

struct MainPSIn
{
	float4 p [[position]];
	float4 t;
	float4 ti;
	float4 c [[function_constant(IIP)]];
	float4 fc [[flat, function_constant(NOT_IIP)]];
	float inv_cov [[function_constant(PS_COVERAGE)]];
	uint interior [[function_constant(PS_INTERIOR)]];
};

struct MainResult
{
	float4 c0;
	float4 c1;
	float depth;
};

struct MainPSOut
{
	float4 c0 [[color(0), index(0), function_constant(PS_OUTPUT_COLOR0)]];
	float4 c1 [[color(0), index(1), function_constant(PS_OUTPUT_COLOR1)]];
	float depthColor [[color(1), function_constant(PS_ZOUTPUT_COLOR)]];
	float depthLess [[depth(less), function_constant(PS_ZOUTPUT_LESS)]];
	float depthAny  [[depth(any),  function_constant(PS_ZOUTPUT_ANY)]];
	MainPSOut(MainResult res)
	{
		if (PS_OUTPUT_COLOR0)
			c0 = res.c0;
		if (PS_OUTPUT_COLOR1)
			c1 = res.c1;
		if (PS_ZOUTPUT_LESS)
			depthLess = res.depth;
		if (PS_ZOUTPUT_ANY)
			depthAny = res.depth;
		if (PS_ZOUTPUT_COLOR)
			depthColor = res.depth;
	}
};

// MARK: - Vertex functions

vertex MainVSOut vs_main(MainVSIn v [[stage_in]], constant GSMTLMainVSUniform& cb [[buffer(GSMTLBufferIndexHWUniforms)]])
{
	return vs_main_impl(v, cb.vertex_scale, cb.vertex_offset, cb.texture_offset, cb.texture_scale);
}

static MainVSIn load_vertex(device const GSMTLMainVertex* vertices, uint idx)
{
	GSMTLMainVertex base = vertices[idx];
	MainVSIn out;
	out.st = base.st;
	out.c = float4(base.rgba);
	out.q = base.q;
	out.p = uint2(base.xy);
	out.z = base.z;
	out.uv = uint2(base.uv);
	out.f = float4(static_cast<float>(base.fog) / 255.f);
	return out;
}

static uint load_index(device const ushort* indices [[buffer(GSMTLBufferIndexHWIndices)]], uint idx)
{
	return indices[idx];
}

#include "tfx_vs.inc"

vertex MainVSOut vs_main_expand(
	uint vid [[vertex_id]],
	device const GSMTLMainVertex* vertices [[buffer(GSMTLBufferIndexHWVertices)]],
	constant GSMTLMainVSUniform& cb [[buffer(GSMTLBufferIndexHWUniforms)]],
	device const ushort* indices [[buffer(GSMTLBufferIndexHWIndices), function_constant(VS_NEEDS_INDEX_BUFFER)]])
{
	return vs_expand_impl(vertices, indices, vid, cb);
}

// MARK: - Fragment functions

struct PSMain
{
	texture2d<float> tex;
	depth2d<float> tex_depth;
	texture2d<float> palette;
	texture2d<float> prim_id_tex;
	sampler tex_sampler;
	float4 current_color;
	float current_depth;
	uint prim_id;
	bool color_discarded = false;
	bool depth_discarded = false;
	const thread MainPSIn& in;
	constant GSMTLMainPSUniform& cb;
	PSUnifformsGeneric cb_generic;

	PSMain(const thread MainPSIn& in, constant GSMTLMainPSUniform& cb): in(in), cb(cb) {}

	
};

// FIXME: Can we just use this struct instead of PSStateGeneric ?

#if FBFETCH_SUPPORT
fragment float4 fbfetch_test(float4 in [[color(0), raster_order_group(0)]])
{
	return in * 2;
}

constant bool NEEDS_RT_TEX = NEEDS_RT && !HAS_FBFETCH && !PS_ROV_COLOR;
constant bool NEEDS_RT_FBF = NEEDS_RT &&  HAS_FBFETCH && !PS_ROV_COLOR;
constant bool NEEDS_DS_FBF = SW_DEPTH &&  HAS_FBFETCH && !DEPTH_FEEDBACK && PS_ROV_DEPTH == ROV_DEPTH::NONE;
#else
constant bool NEEDS_RT_TEX = NEEDS_RT && !PS_ROV_COLOR;
constant bool NEEDS_DS_FBF = false;
constant float ds_fbf = 0;
#endif
constant bool NEEDS_DS_TEX   = SW_DEPTH && !DEPTH_FEEDBACK && !NEEDS_DS_FBF && PS_ROV_DEPTH == ROV_DEPTH::NONE;
constant bool NEEDS_DS_DEPTH = (SW_DEPTH && DEPTH_FEEDBACK || NEEDS_DS_FBF) && PS_ROV_DEPTH == ROV_DEPTH::NONE;
constant bool NEEDS_RT_ROV = PS_ROV_COLOR && !ROV_NEEDS_R32;
constant bool NEEDS_RT_U32 = PS_ROV_COLOR &&  ROV_NEEDS_R32;
constant bool NEEDS_DS_ROV = PS_ROV_DEPTH != ROV_DEPTH::NONE;

fragment MainPSOut ps_main(
	MainPSIn in [[stage_in]],
	constant GSMTLMainPSUniform& cb [[buffer(GSMTLBufferIndexHWUniforms)]],
	sampler s [[sampler(0)]],
#if PRIMID_SUPPORT
	uint primid [[primitive_id, function_constant(NEEDS_PRIMID)]],
#endif
#if FBFETCH_SUPPORT
	float4 rt_fbf [[color(0), raster_order_group(0), function_constant(NEEDS_RT_FBF)]],
	float  ds_fbf [[color(1), raster_order_group(1), function_constant(NEEDS_DS_FBF)]],
#endif
	texture2d<float> tex       [[texture(GSMTLTextureIndexTex),          function_constant(PS_TEX_IS_COLOR)]],
	depth2d<float>   depth     [[texture(GSMTLTextureIndexTex),          function_constant(PS_TEX_IS_DEPTH)]],
	texture2d<float> palette   [[texture(GSMTLTextureIndexPalette),      function_constant(PS_HAS_PALETTE)]],
	texture2d<float> rt        [[texture(GSMTLTextureIndexRenderTarget), function_constant(NEEDS_RT_TEX)]],
	texture2d<float> primidtex [[texture(GSMTLTextureIndexPrimIDs),      function_constant(PS_PRIM_CHECKING_READ)]],
	texture2d<float> ds_tex    [[texture(GSMTLTextureIndexDepthTarget),  function_constant(NEEDS_DS_TEX)]],
	depth2d<float>   ds_depth  [[texture(GSMTLTextureIndexDepthTarget),  function_constant(NEEDS_DS_DEPTH)]],
	texture2d<float, access::read_write> rt_rov [[texture(GSMTLTextureIndexRenderTarget), raster_order_group(0), function_constant(NEEDS_RT_ROV)]],
	texture2d<uint,  access::read_write> rt_u32 [[texture(GSMTLTextureIndexRenderTarget), raster_order_group(0), function_constant(NEEDS_RT_U32)]],
	texture2d<float, access::read_write> ds_rov [[texture(GSMTLTextureIndexDepthTarget),  raster_order_group(1), function_constant(NEEDS_DS_ROV)]])
{
	PSStateGeneric state;

	state.in = in;
	state.cb = cb;

	state.tex_sampler = s;
	if (PS_TEX_IS_COLOR)
		state.tex = tex;
	else
		state.tex_depth = depth;
	if (PS_HAS_PALETTE)
		state.palette = palette;
	if (PS_PRIM_CHECKING_READ)
		state.prim_id_tex = primidtex;
#if PRIMID_SUPPORT
	if (NEEDS_PRIMID)
		state.prim_id = primid;
#endif

	uint2 coord = uint2(in.p.xy);

	if (SW_DEPTH)
	{
		if (PS_ROV_DEPTH != ROV_DEPTH::NONE)
			main.current_depth = ds_rov.read(coord).x;
		else if (DEPTH_FEEDBACK)
			main.current_depth = ds_depth.read(coord);
		else if (NEEDS_DS_FBF)
			main.current_depth = ds_fbf < 0 ? ds_depth.read(coord) : ds_fbf;
		else
			main.current_depth = ds_tex.read(coord).x;
	}

	if (NEEDS_RT || (PS_ROV_COLOR && any(cb.fbmask == 0xff)))
	{
		if (PS_ROV_COLOR)
		{
			if (ROV_NEEDS_R32)
				main.current_color = unpack_unorm4x8_to_float(rt_u32.read(coord).x);
			else
				main.current_color = rt_rov.read(coord);
		}
		else
		{
#if FBFETCH_SUPPORT
			main.current_color = HAS_FBFETCH ? rt_fbf : rt.read(coord);
#else
			main.current_color = rt.read(coord);
#endif
		}
	}
	else
	{
		main.current_color = 0;
	}

	MainResult out = main.ps_main();
	if (PS_ROV_DEPTH == ROV_DEPTH::READ_WRITE && !main.depth_discarded)
		ds_rov.write(out.depth, coord);
	if (PS_ROV_COLOR && !main.color_discarded)
	{
		if (!PS_FBMASK)
			out.c0 = select(out.c0, main.current_color, cb.fbmask == 0xff);
		if (ROV_NEEDS_R32)
			rt_u32.write(pack_float_to_unorm4x8(out.c0), coord);
		else
			rt_rov.write(out.c0, coord);
	}
	return out;
}

// Metal doesn't let you toggle eft with function constants so we need a separate function for it
[[early_fragment_tests]]
fragment void ps_main_rov_eft(
	MainPSIn in [[stage_in]],
	constant GSMTLMainPSUniform& cb [[buffer(GSMTLBufferIndexHWUniforms)]],
	sampler s [[sampler(0)]],
	texture2d<float> tex     [[texture(GSMTLTextureIndexTex),     function_constant(PS_TEX_IS_COLOR)]],
	depth2d<float>   depth   [[texture(GSMTLTextureIndexTex),     function_constant(PS_TEX_IS_DEPTH)]],
	texture2d<float> palette [[texture(GSMTLTextureIndexPalette), function_constant(PS_HAS_PALETTE)]],
	texture2d<float, access::read_write> rt_rov [[texture(GSMTLTextureIndexRenderTarget), raster_order_group(0), function_constant(NEEDS_RT_ROV)]],
	texture2d<uint,  access::read_write> rt_u32 [[texture(GSMTLTextureIndexRenderTarget), raster_order_group(0), function_constant(NEEDS_RT_U32)]])
{
	PSMain main(in, cb);
	main.tex_sampler = s;
	if (PS_TEX_IS_COLOR)
		main.tex = tex;
	else
		main.tex_depth = depth;
	if (PS_HAS_PALETTE)
		main.palette = palette;

	uint2 coord = uint2(in.p.xy);
	if (ROV_NEEDS_R32)
		main.current_color = unpack_unorm4x8_to_float(rt_u32.read(coord).x);
	else
		main.current_color = rt_rov.read(coord);
	MainPSOut out = main.ps_main();
	if (!main.color_discarded)
	{
		if (!PS_FBMASK)
			out.c0 = select(out.c0, main.current_color, cb.fbmask == 0xff);
		if (ROV_NEEDS_R32)
			rt_u32.write(pack_float_to_unorm4x8(out.c0), coord);
		else
			rt_rov.write(out.c0, coord);
	}
}

#if PRIMID_SUPPORT
fragment uint primid_test(uint id [[primitive_id]])
{
	return id;
}
#endif

// MARK: Markers for detecting the Metal version a metallib was compiled against

#if __METAL_VERSION__ >= 210
kernel void metal_version_21() {}
#endif
#if __METAL_VERSION__ >= 220
kernel void metal_version_22() {}
#endif
#if __METAL_VERSION__ >= 230
kernel void metal_version_23() {}
#endif

// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

// FIXME: Remove after done.
#version 460 core
#extension GL_EXT_samplerless_texture_functions : require
#define HAS_FEEDBACK_LOOP_LAYOUT 1
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_ARB_shader_image_load_store : require
#define FRAGMENT_SHADER 1

#define FLOAT2 vec2
#define FLOAT3 vec3
#define FLOAT4 vec4
#define FLOAT2x2 mat2x2
#define FLOAT2x4 mat2x4
#define FLOAT4x4 mat4x4
#define UINT2 uvec2
#define UINT3 uvec3
#define UINT4 uvec4
#define INT2 ivec2
#define INT3 ivec3
#define INT4 ivec4
#define USHORT uint
#define USHORT2 uvec2
#define USHORT3 uvec3
#define USHORT4 uvec4
#define SHORT int
#define SHORT2 ivec2
#define SHORT3 ivec3
#define SHORT4 ivec4
#define BOOL2 bvec2
#define BOOL3 bvec3
#define BOOL4 bvec4

#define STATIC
#define DFDX dFdx
#define DFDY dFdy

#define SELECT(COND, TRUE_VAL, FALSE_VAL) mix((COND), (FALSE_VAL), (TRUE_VAL))
#define VEQUAL(X, Y) equal((X), (Y))
#define VGEQUAL(X, Y) greaterThanEqual((X), (Y))
#define VLEQUAL(X, Y) lessThanEqual((X), (Y))
#define VGREATER(X, Y) greaterThan((X), (Y))
#define VLESS(X, Y) lessThan((X), (Y))
#define VNOTEQUAL(X, Y) notEqual((X), (Y))
#define RSQRT(X) inversesqrt(X)
#define GPU_DISCARD discard
#define LEVEL(X) (X)
#define SATURATE(X) clamp((X), 0.0f, 1.0f)
#define FLOAT_BITCAST_UINT(X) floatBitsToUint(X)
#define UINT_BITCAST_FLOAT4(X) FLOAT4((X) & 0xFFu, ((X) >> 8) & 0xFFu, ((X) >> 16) & 0xFFu, ((X) >> 24) & 0xFFu)

#define VERTICES_PARAM(NAME) uint NAME
#define INDICES_PARAM(NAME) uint NAME
#define IN_PARAM(TYPE, NAME) TYPE NAME
#define OUT_PARAM(TYPE, NAME) out TYPE NAME
#define IN_OUT_PARAM(TYPE, NAME) inout TYPE NAME

#define PRIMID_MAX 0x7FFFFFFF
#define VS_Y_FLIP 1.0f
#define EXP2_MIN_32 exp2(-32.0f)
#define EXP2_POS_32 exp2(32.0f)

#define PS_UV_MSK_FIX(CB) floatBitsToUint(CB.uv_min_max)
#define PS_SAMPLE_TEX(STATE, POS) texture(Texture, FLOAT2(POS))
#define PS_SAMPLE_TEX_LOD(STATE, POS, LOD) textureLod(Texture, FLOAT2(POS), float(LOD))
#define PS_SAMPLE_TEX_DEPTH(STATE, POS) PS_SAMPLE_TEX(STATE, (POS))
#define PS_SAMPLE_TEX_DEPTH_LOD(STATE, POS, LOD) PS_SAMPLE_TEX_LOD(STATE, (POS), (LOD))
#define PS_READ_TEX(STATE, POS, LOD) texelFetch(Texture, INT2(POS), int(LOD))
#define PS_READ_TEX_DEPTH(STATE, POS, LOD) PS_READ_TEX(STATE, (POS), (LOD))
#define PS_READ_PALETTE(STATE, POS) texelFetch(Palette, INT2(POS), 0)
#define PS_READ_PRIMID(STATE, POS) texelFetch(PrimMinTexture, INT2(POS), 0)
#define PS_GET_TEX_DIMS(STATE) UINT2(textureSize(Texture, 0))
#define PS_GET_TEX_DEPTH_DIMS(STATE) PS_GET_TEX_DIMS(STATE)

#ifndef VS_EXPAND_NONE
#define VS_EXPAND_NONE 0
#define VS_EXPAND_POINT 1
#define VS_EXPAND_LINE 2
#define VS_EXPAND_SPRITE 3
#define VS_EXPAND_LINE_AA1 4
#define VS_EXPAND_TRIANGLE_AA1 5
#endif

#ifndef ZTST_GEQUAL
#define ZTST_GEQUAL 2
#define ZTST_GREATER 3
#endif

#ifndef AFAIL_KEEP
#define AFAIL_KEEP 0
#define AFAIL_FB_ONLY 1
#define AFAIL_ZB_ONLY 2
#define AFAIL_RGB_ONLY 3
#define AFAIL_RGB_ONLY_DSB 4
#define AFAIL_RGB_ONLY_SW_Z 5
#endif

#ifndef PS_ATST_NONE
#define PS_ATST_NONE 0
#define PS_ATST_LEQUAL 1
#define PS_ATST_GEQUAL 2
#define PS_ATST_EQUAL 3
#define PS_ATST_NOTEQUAL 4
#endif

#ifndef PS_AA1_NONE
#define PS_AA1_NONE 0
#define PS_AA1_LINE 1
#define PS_AA1_TRIANGLE 2
#define PS_AA1_TRIANGLE_SW_Z 3
#endif

#ifndef PS_ROV_DEPTH_NONE
#define PS_ROV_DEPTH_NONE 0
#define PS_ROV_DEPTH_READ_WRITE 1
#define PS_ROV_DEPTH_READ_ONLY 2
#endif

#define FMT_32 0
#define FMT_24 1
#define FMT_16 2

#define SHUFFLE_READ  1
#define SHUFFLE_WRITE 2
#define SHUFFLE_READWRITE 3

#define LOAD_VERTEX(VERTICES, VID) load_vertex(VID)
#define LOAD_INDEX(INDICES, VID) load_index(VID)

#include "tfx_defs.inc"

//////////////////////////////////////////////////////////////////////
// Vertex Shader
//////////////////////////////////////////////////////////////////////

#if defined(VERTEX_SHADER)

#ifndef VS_TME
#define VS_TME 0
#define VS_FST 0
#define VS_IIP 0
#define VS_POINT_SIZE 0
#define VS_EXPAND_TYPE VS_EXPAND_NONE
#endif

// The loading functions must be defined before the generic vertex functions are included.
layout(push_constant) uniform cb2
{
	uint BaseVertex;
	uint BaseIndex;
	uint pad_cb2_0;
	uint pad_cb2_1;
};

struct RawVertex
{
	vec2 ST;
	uint RGBA;
	float Q;
	uint XY;
	uint Z;
	uint UV;
	uint FOG;
};

layout(std140, set = 0, binding = 2) readonly buffer VertexBuffer {
	RawVertex vertex_buffer[];
};

// Warning: use std430 instead of std140 so that the ints are tightly packed.
layout(std430, set = 0, binding = 3) readonly buffer IndexBuffer {
	uint index_buffer[];
};

uint load_index(uint _i)
{
	uint i = _i + BaseIndex;
	// i is even => load lower 16 bits; i odd => load upper 16 bits.
	uint shift = (i & 1u) << 4u;
	return (index_buffer[i >> 1u] >> shift) & 0xFFFFu;
}

VSInputGeneric load_vertex(uint index)
{
	RawVertex rvtx = vertex_buffer[BaseVertex + index];

	vec2 a_st = rvtx.ST;
	uvec4 a_c = uvec4(bitfieldExtract(rvtx.RGBA, 0, 8), bitfieldExtract(rvtx.RGBA, 8, 8),
	                  bitfieldExtract(rvtx.RGBA, 16, 8), bitfieldExtract(rvtx.RGBA, 24, 8));
	float a_q = rvtx.Q;
	uvec2 a_p = uvec2(bitfieldExtract(rvtx.XY, 0, 16), bitfieldExtract(rvtx.XY, 16, 16));
	uint a_z = rvtx.Z;
	uvec2 a_uv = uvec2(bitfieldExtract(rvtx.UV, 0, 16), bitfieldExtract(rvtx.UV, 16, 16));
	vec4 a_f = unpackUnorm4x8(rvtx.FOG);

	VSInputGeneric v;
  v.st = a_st;
  v.c = FLOAT4(a_c);
  v.q = a_q;
  v.p = a_p;
  v.z = a_z;
  v.uv = a_uv;
  v.f = a_f;

  return v;
}

// Include generic functions.
#include "tfx_vs.inc"

layout(std140, set = 0, binding = 0) uniform cb0
{
	vec2 VertexScale;
	vec2 VertexOffset;
	vec2 TextureScale;
	vec2 TextureOffset;
	vec2 PointSize;
	uint MaxDepth;
	float LineAA1Width;
};

VSUniformsGeneric GetVSUniforms()
{
  VSUniformsGeneric cb;
  cb.vertex_scale = VertexScale;
  cb.vertex_offset = VertexOffset;
  cb.texture_scale = TextureScale;
  cb.texture_offset = TextureOffset;
  cb.point_size = PointSize;
  cb.max_depth = MaxDepth;
  cb.line_aa1_width = LineAA1Width;
  return cb;
}

layout(location = 0) out VSOutput
{
	vec4 t;
	vec4 ti;

	#if VS_IIP != 0
		vec4 c;
	#else
		flat vec4 c;
	#endif

	float inv_cov; // We use the inverse to make it simpler to interpolate.
	flat uint interior; // 1 for triangle interior; 0 for edge;
} vsOut;

void WriteVSOutput(VSOutputGeneric v)
{
  gl_Position = v.p;
  vsOut.t = v.t;
  vsOut.ti = v.ti;
  vsOut.c = v.c;
  vsOut.inv_cov = v.inv_cov;
  vsOut.interior = v.interior;
  gl_PointSize = v.point_size;
}

#if VS_EXPAND == VS_EXPAND_NONE

layout(location = 0) in vec2 a_st;
layout(location = 1) in uvec4 a_c;
layout(location = 2) in float a_q;
layout(location = 3) in uvec2 a_p;
layout(location = 4) in uint a_z;
layout(location = 5) in uvec2 a_uv;
layout(location = 6) in vec4 a_f;

VSInputGeneric GetVSInput()
{
  VSInputGeneric vsinput;
  vsinput.st = a_st;
  vsinput.c = FLOAT4(a_c);
  vsinput.q = a_q;
  vsinput.p = a_p;
  vsinput.z = a_z;
  vsinput.uv = a_uv;
  vsinput.f = a_f;
  return vsinput;
}

void main()
{
  VSInputGeneric vsinput = GetVSInput();
  VSUniformsGeneric cb = GetVSUniforms();
	VSOutputGeneric vout = vs_main_impl(vsinput, cb);
  WriteVSOutput(vout);
}

#else // VS_EXPAND

void main()
{
	uint vid = uint(gl_VertexIndex);
  VSUniformsGeneric cb = GetVSUniforms();
  VSOutputGeneric vout = vs_expand_impl(0, 0, vid, cb);
  WriteVSOutput(vout);
}

#endif // VS_EXPAND

#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 0
#define PS_WMT 0
#define PS_ADJS 0
#define PS_ADJT 0
#define PS_FMT FMT_32
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 1
#define PS_ATST 1
#define PS_AFAIL 0
#define PS_FOG 0
#define PS_BLEND_HW 0
#define PS_A_MASKED 0
#define PS_FBA 0
#define PS_FBMASK 0
#define PS_LTF 1
#define PS_TCOFFSETHACK 0
#define PS_SHUFFLE 0
#define PS_SHUFFLE_SAME 0
#define PS_PROCESS_BA 0
#define PS_PROCESS_RG 0
#define PS_SHUFFLE_ACROSS 0
#define PS_WRITE_RG 0
#define PS_READ16_SRC 0
#define PS_DST_FMT 0
#define PS_DEPTH_FMT 0
#define PS_PAL_FMT 0
#define PS_CHANNEL_FETCH 0
#define PS_TALES_OF_ABYSS_HLE 0
#define PS_URBAN_CHAOS_HLE 0
#define PS_COLCLIP_HW 0
#define PS_COLCLIP 0
#define PS_BLEND_A 0
#define PS_BLEND_B 0
#define PS_BLEND_C 0
#define PS_BLEND_D 0
#define PS_FIXED_ONE_A 0
#define PS_PABE 0
#define PS_DITHER 0
#define PS_DITHER_ADJUST 0
#define PS_ZCLAMP 0
#define PS_ZFLOOR 0
#define PS_SCANMSK 0
#define PS_AUTOMATIC_LOD 0
#define PS_MANUAL_LOD 0
#define PS_TEX_IS_FB 0
#define PS_NO_COLOR 0
#define PS_NO_COLOR1 0
#define PS_DATE 0
#define PS_ROV_COLOR 0
#define PS_ROV_DEPTH 0
#define PS_SW_ANISO 0
#define PS_REGION_RECT 0
#define PS_RTA_SRC_CORRECTION 0
#define PS_AEM_FMT 0
#define PS_CHANNEL 0
#define PS_IIP 0
#define PS_ROUND_INV 0
#define PS_BLEND_MIX 0
#define PS_RTA_CORRECTION 0
#define PS_ZTST 0
#define PS_AA1 0
#define PS_ABE 0
#endif

#define PS_TEX_IS_DEPTH 0

#define SW_BLEND (PS_BLEND_A != 0 || PS_BLEND_B != 0 || PS_BLEND_D != 0)
#define SW_BLEND_NEEDS_RT (SW_BLEND && (PS_BLEND_A == 1 || PS_BLEND_B == 1 || PS_BLEND_C == 1 || PS_BLEND_D == 1))
#define SW_AD_TO_HW (PS_BLEND_C == 1 && PS_A_MASKED != FALSE)
#define AFAIL_NEEDS_RT (PS_AFAIL == AFAIL_ZB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY_SW_Z)
#define AFAIL_NEEDS_DEPTH (PS_AFAIL == AFAIL_FB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY_SW_Z)
#define ZTST_NEEDS_DEPTH (PS_ZTST == ZTST_GEQUAL || PS_ZTST == ZTST_GREATER)
#define AA1_NEEDS_DEPTH (PS_AA1 == PS_AA1_TRIANGLE_SW_Z)

#define PS_FEEDBACK_LOOP_IS_NEEDED_RT (PS_TEX_IS_FB != FALSE || AFAIL_NEEDS_RT || PS_FBMASK != FALSE || SW_BLEND_NEEDS_RT || SW_AD_TO_HW || (PS_DATE >= 5))
#define PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH (AFAIL_NEEDS_DEPTH || ZTST_NEEDS_DEPTH || AA1_NEEDS_DEPTH)
#define ZWRITE (PS_ZCLAMP || PS_ZFLOOR || PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH)

#define PS_RETURN_COLOR_ROV (!PS_NO_COLOR && PS_ROV_COLOR)
#define PS_RETURN_COLOR (!PS_NO_COLOR && !PS_ROV_COLOR)
#define PS_RETURN_DEPTH_ROV (PS_ROV_DEPTH == PS_ROV_DEPTH_READ_WRITE)
#define PS_RETURN_DEPTH (ZWRITE && !PS_ROV_DEPTH)
#define PS_ROV_EARLYDEPTHSTENCIL (PS_ROV_COLOR && !PS_ROV_DEPTH && !ZWRITE)

#define NEEDS_TEX (PS_TFX != 4)
#define NEEDS_RT PS_FEEDBACK_LOOP_IS_NEEDED_RT
#define USE_FEEDBACK_SAMPLER (DISABLE_TEXTURE_BARRIER || HAS_FEEDBACK_LOOP_LAYOUT)

layout(std140, set = 0, binding = 1) uniform cb1
{
	vec3 FogColor;
	float AREF;
	vec4 WH;
	vec2 TA;
	float MaxDepthPS;
	float Af;
	uvec4 FbMask;
	vec4 HalfTexel;
	vec4 MinMax;
	vec4 LODParams;
	vec4 STRange;
	ivec4 ChannelShuffle;
	vec2 ChannelShuffleOffset;
	vec2 TC_OffsetHack;
	vec2 STScale;
	mat4 DitherMatrix;
	float ScaledScaleFactor;
	float RcpScaleFactor;
	float _pad0_cb1;
	float _pad1_cb1;
	float LineCovScale;
	float _pad2_cb1;
	float _pad3_cb1;
	float _pad4_cb1;
};

PSUniformsGeneric GetPSUniforms()
{
  PSUniformsGeneric cb;

  cb.fog_color = FogColor;
  cb.aref = AREF;
	cb.wh = WH;
	cb.ta = TA;
	cb.max_depth = MaxDepthPS;
	cb.alpha_fix = Af;
	cb.fbmask = FbMask;

	cb.half_texel = HalfTexel;
  cb.uv_min_max = MinMax;
	cb.lod_params = LODParams;
	cb.st_range = STRange;
  
  cb.channel_shuffle_blue_mask = ChannelShuffle.x;
  cb.channel_shuffle_blue_shift = ChannelShuffle.y;
  cb.channel_shuffle_green_mask = ChannelShuffle.z;
  cb.channel_shuffle_green_shift = ChannelShuffle.w;

	cb.channel_shuffle_offset = ChannelShuffleOffset;
	cb.tc_offset = TC_OffsetHack;
	cb.st_scale = STScale;
	cb.dither_matrix = DitherMatrix;

	cb.scale_factor = FLOAT4(ScaledScaleFactor, RcpScaleFactor, 0.0f, 0.0f);

	cb.line_cov_scale = LineCovScale;

  return cb;
}

layout(location = 0) in VSOutput
{
	vec4 t;
	vec4 ti;
	#if PS_IIP != 0
		vec4 c;
	#else
		flat vec4 c;
	#endif
	float inv_cov; // We use the inverse to make it simpler to interpolate.
	flat uint interior; // 1 for triangle interior; 0 for edge;
} vsIn;

PSInputGeneric GetPSInput()
{
  PSInputGeneric psinput;
  psinput.p = gl_FragCoord;
  psinput.t = vsIn.t;
  psinput.ti = vsIn.ti;
  psinput.c = vsIn.c;
  psinput.fc = vsIn.c;
  psinput.inv_cov = vsIn.inv_cov;
  psinput.interior = vsIn.interior;
  return psinput;
}

#if PS_RETURN_COLOR
	#if !PS_NO_COLOR1
		layout(location = 0, index = 0) out vec4 o_col0;
		layout(location = 0, index = 1) out vec4 o_col1;
	#elif !PS_NO_COLOR
		layout(location = 0) out vec4 o_col0;
	#endif
#elif PS_RETURN_COLOR_ROV
	vec4 o_col0;
#endif

layout(set = 1, binding = 0) uniform sampler2D Texture;
layout(set = 1, binding = 1) uniform texture2D Palette;
layout(set = 1, binding = 3) uniform texture2D PrimMinTexture;
layout(set = 1, binding = 5, rgba8) uniform restrict coherent image2D RtImageRov;
layout(set = 1, binding = 6, r32f) uniform restrict coherent image2D DepthImageRov;

#if USE_FEEDBACK_SAMPLER
	layout(set = 1, binding = 2) uniform texture2D RtSampler;
	layout(set = 1, binding = 4) uniform texture2D DepthSampler;
#else
	// Must consider each case separately since the input attachment indices must be consecutive.
	#if (PS_FEEDBACK_LOOP_IS_NEEDED_RT && !PS_ROV_COLOR) && (PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && !PS_ROV_DEPTH)
		layout(input_attachment_index = 0, set = 1, binding = 2) uniform subpassInput RtSampler;
		layout(input_attachment_index = 1, set = 1, binding = 4) uniform subpassInput DepthSampler;
	#elif (PS_FEEDBACK_LOOP_IS_NEEDED_RT && !PS_ROV_COLOR)
		layout(input_attachment_index = 0, set = 1, binding = 2) uniform subpassInput RtSampler;
	#elif (PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && !PS_ROV_DEPTH)
		layout(input_attachment_index = 0, set = 1, binding = 4) uniform subpassInput DepthSampler;
	#endif
#endif

#if ZWRITE && !PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH
layout(depth_less) out float gl_FragDepth;
#endif

#if PS_ROV_COLOR || PS_ROV_DEPTH
layout(pixel_interlock_ordered) in;
#endif

#if PS_ROV_EARLYDEPTHSTENCIL
layout(early_fragment_tests) in;
#endif

vec4 RtLoad(ivec2 xy)
{
	#if PS_ROV_COLOR
		return imageLoad(RtImageRov, xy);
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_RT && USE_FEEDBACK_SAMPLER
		return texelFetch(RtSampler, xy, 0);
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_RT
		return subpassLoad(RtSampler);
	#else
		return vec4(0.0f);
	#endif
}

float DepthLoad(ivec2 xy)
{
	#if PS_ROV_COLOR
		return imageLoad(DepthImageRov, xy).r;
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && USE_FEEDBACK_SAMPLER
		return texelFetch(DepthSampler, xy, 0).r;
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH
		return subpassLoad(DepthSampler).r;
	#else
		return 0.0f;
	#endif
}

#include "tfx_ps.inc"

void main()
{
	float input_z = gl_FragCoord.z;

  PSMain state;
  state.psinput = GetPSInput();
  state.cb = GetPSUniforms();
  state.tex = 0;
  state.tex_depth = 0;
  state.palette = 0;
  state.prim_id_tex = 0;
  state.prim_id = gl_PrimitiveID;
  state.color_discarded = false;
  state.depth_discarded = false;

  ivec2 coord = ivec2(state.psinput.p.xy);

  #if PS_ROV_COLOR || PS_ROV_DEPTH
    beginInvocationInterlockARB();
  #endif

	state.current_depth = DepthLoad(coord);

	state.current_color = RtLoad(coord);

  PSOutputGeneric psout = ps_main_impl(state);
	
	// Writing back color (result already written to o_col0 for non-ROV)
	#if PS_RETURN_COLOR_ROV
		psout.c0 = mix(psout.c0, state.current_color, equal(FbMask, uvec4(0xFFu))); // channel masking

		if (!state.color_discarded)
			imageStore(RtImageRov, coord, psout.c0);
	#endif
	
	// Writing back depth
	#if PS_RETURN_DEPTH
		gl_FragDepth = psout.depth;
	#elif PS_RETURN_DEPTH_ROV
		if (!state.depth_discarded)
			imageStore(DepthImageRov, coord, vec4(psout.depth, 0, 0, 1.0f));
	#endif

	#if PS_ROV_COLOR || PS_ROV_DEPTH
		endInvocationInterlockARB();
	#endif
}

#endif

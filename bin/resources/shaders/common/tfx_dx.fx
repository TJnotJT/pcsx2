// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

/// Start helper macros for shared shader code.
#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT4 float4
#define FLOAT2x2 float2x2
#define FLOAT2x4 float2x4
#define FLOAT4x4 float4x4
#define UINT2 uint2
#define UINT3 uint3
#define UINT4 uint4
#define INT2 int2
#define INT3 int3
#define INT4 int4
#define USHORT uint
#define USHORT2 uint2
#define USHORT3 uint3
#define USHORT4 uint4
#define SHORT int
#define SHORT2 int2
#define SHORT3 int3
#define SHORT4 int4
#define BOOL2 bool2
#define BOOL3 bool3
#define BOOL4 bool4

#define STATIC
#define DFDX ddx
#define DFDY ddy

#define SELECT(COND, TRUE_VAL, FALSE_VAL) ((COND) ? (FALSE_VAL) : (TRUE_VAL))
#define VEQUAL(X, Y) ((X) == (Y))
#define VGEQUAL(X, Y) ((X) >= (Y))
#define VLEQUAL(X, Y) ((X) <= (Y))
#define VGREATER(X, Y) ((X) > (Y))
#define VLESS(X, Y) ((X) < (Y))
#define VNOTEQUAL(X, Y) ((X) != (Y))
#define RSQRT(X) rsqrt(X)
#define GPU_DISCARD discard
#define LEVEL(X) (X)
#define SATURATE(X) saturate(X)
#define FLOAT_BITCAST_UINT(X) asuint(X)
#define UINT_BITCAST_UCHAR4(X) UINT4((X) & 0xFFu, ((X) >> 8) & 0xFFu, ((X) >> 16) & 0xFFu, ((X) >> 24) & 0xFFu)
#define MAT_MUL(X, Y) mul((Y), (X)) // Warning: operands opposite order of GLSL and MSL!
#define FRACT(X) frac(X)
#define MIX lerp

#define VERTICES_PARAM(NAME) uint NAME
#define INDICES_PARAM(NAME) uint NAME
#define IN_PARAM(TYPE, NAME) TYPE NAME
#define IN_OUT_PARAM(TYPE, NAME) inout TYPE NAME

#define PRIMID_MAX 0x7FFFFFFF
#define VS_Y_FLIP -1.0f
#define EXP2_MIN_32 exp2(-32.0f)
#define EXP2_POS_32 exp2(32.0f)

#define VS_SCALE_RAW_Z(Z) (float(Z) * EXP2_MIN_32)

#define PS_UV_MSK_FIX(CB) (FLOAT_BITCAST_UINT(CB.uv_min_max))
#define PS_SAMPLE_TEX(STATE, POS) (Texture.Sample(TextureSampler, FLOAT2(POS)))
#define PS_SAMPLE_TEX_LOD(STATE, POS, LOD) (Texture.SampleLevel(TextureSampler, FLOAT2(POS), float(LOD)))
#define PS_SAMPLE_TEX_DEPTH(STATE, POS) (PS_SAMPLE_TEX(STATE, (POS)).r)
#define PS_SAMPLE_TEX_DEPTH_LOD(STATE, POS, LOD) (PS_SAMPLE_TEX_LOD(STATE, (POS), (LOD)).r)
#define PS_READ_TEX(STATE, POS, LOD) (Texture.Load(INT3(INT2(POS), int(LOD))))
#define PS_READ_TEX_DEPTH(STATE, POS, LOD) (PS_READ_TEX(STATE, (POS), (LOD)).r)
#define PS_READ_PALETTE(STATE, POS) (Palette.Load(INT3(INT2(POS), 0)))
#define PS_READ_PRIMID(STATE, POS) (PrimMinTexture.Load(INT3(INT2(POS), 0)).r)
#define PS_GET_TEX_DIMS(STATE, OUT_VAR) (Texture.GetDimensions(OUT_VAR.x, OUT_VAR.y))
#define PS_GET_TEX_DEPTH_DIMS(STATE, OUT_VAR) (PS_GET_TEX_DIMS(STATE, OUT_VAR))

#define LOAD_VERTEX(VERTICES, VID) load_vertex(VID)
#define LOAD_INDEX(INDICES, VID) load_index(VID)

// DX does use point/line size.
#define VS_POINT_SIZE 0

/// End helper macros for shared shader code.

#include "tfx_defs.inc"

#ifdef VERTEX_SHADER

#ifndef VS_TME
#define VS_TME 0
#define VS_FST 0
#define VS_IIP 0
#define VS_POINT_SIZE 0
#define VS_EXPAND_TYPE 0
#endif

struct VS_INPUT
{
	float2 st : TEXCOORD0;
	uint4 c : COLOR0;
	float q : TEXCOORD1;
	uint2 p : POSITION0;
	uint z : POSITION1;
	uint2 uv : TEXCOORD2;
	float4 f : COLOR1;
};

struct VS_RAW_INPUT
{
	float2 ST;
	uint RGBA;
	float Q;
	uint XY;
	uint Z;
	uint UV;
	uint FOG;
};

struct VS_OUTPUT
{
	float4 p : SV_Position;
	float4 t : TEXCOORD0;
	float4 ti : TEXCOORD2;

#if VS_IIP != 0
	float4 c : COLOR0;
#else
	nointerpolation float4 c : COLOR0;
#endif

	float inv_cov : COLOR1; // We use the inverse to make it simpler to interpolate.
	nointerpolation uint interior : COLOR2; // 1 for triangle interior; 0 for edge;
};

// VS Constant Buffer
#ifdef DX12
cbuffer cb0 : register(b0)
#else
cbuffer cb0
#endif
{
	#define X(TYPE, NAME) TYPE NAME;
		VS_UNIFORMS(X)
	#undef X
};

#ifdef DX12
cbuffer cb2 : register(b2)
#else
cbuffer cb2
#endif
{
	#define X(TYPE, NAME) TYPE NAME;
		VS_PUSH_CONSTANTS(X)
	#undef X
};

StructuredBuffer<VS_RAW_INPUT> vertices : register(t0);
StructuredBuffer<uint> IndexBuffer : register(t5);

VSUniformsGeneric GetVSUniforms()
{
  VSUniformsGeneric cb;
	#define X(TYPE, NAME) cb.NAME = NAME;
		VS_UNIFORMS(X)
	#undef X
  return cb;
}

VSInputGeneric GetVSInput(VS_INPUT vin)
{
  VSInputGeneric vin_gen;
  vin_gen.st = vin.st;
  vin_gen.c = FLOAT4(vin.c);
  vin_gen.q = vin.q;
  vin_gen.p = vin.p;
  vin_gen.z = vin.z;
  vin_gen.uv = vin.uv;
  vin_gen.f = vin.f;
  return vin_gen;
}

VS_OUTPUT GetVSOutput(VSOutputGeneric vout_gen)
{
  VS_OUTPUT vout;
	vout.p = vout_gen.p;
	vout.t = vout_gen.t;
	vout.ti = vout_gen.ti;
	vout.c = vout_gen.c;
	vout.inv_cov = vout_gen.inv_cov;
	vout.interior = vout_gen.interior;
  return vout;
}

uint load_index(uint _i)
{
	uint i = _i + BaseIndex;
	// i is even => load lower 16 bits; i odd => load upper 16 bits.
	uint shift = (i & 1u) << 4u;
	return (IndexBuffer.Load(i >> 1u) >> shift) & 0xFFFFu;
}

VSInputGeneric load_vertex(uint index)
{
	VS_RAW_INPUT raw = vertices.Load(BaseVertex + index);

	VSInputGeneric vert;
	vert.st = raw.ST;
	vert.c = uint4(raw.RGBA & 0xFFu, (raw.RGBA >> 8) & 0xFFu, (raw.RGBA >> 16) & 0xFFu, raw.RGBA >> 24);
	vert.q = raw.Q;
	vert.p = uint2(raw.XY & 0xFFFFu, raw.XY >> 16);
	vert.z = raw.Z;
	vert.uv = uint2(raw.UV & 0xFFFFu, raw.UV >> 16);
	vert.f = float4(float(raw.FOG & 0xFFu), float((raw.FOG >> 8) & 0xFFu), float((raw.FOG >> 16) & 0xFFu), float(raw.FOG >> 24)) / 255.0f;
	return vert;
}

#include "tfx_vs.inc"

#if VS_EXPAND_TYPE == VS_EXPAND_NONE

VS_OUTPUT vs_main(VS_INPUT vin)
{
	VSInputGeneric vin_gen = GetVSInput(vin);
  VSUniformsGeneric cb = GetVSUniforms();
	VSOutputGeneric vout_gen = vs_main_impl(vin_gen, cb);
	return GetVSOutput(vout_gen);
}

#else // VS_EXPAND_TYPE

VS_OUTPUT vs_main_expand(uint vid : SV_VertexID)
{
  VSUniformsGeneric cb = GetVSUniforms();
  VSOutputGeneric vout_gen = vs_expand_impl(vid, 0, cb, 0);
  return GetVSOutput(vout_gen);
}

#endif // VS_EXPAND

#endif // VERTEX_SHADER

#ifdef PIXEL_SHADER

#define PS_PRIM_CHECKING_INIT (PS_DATE == 1 || PS_DATE == 2)
#define PS_TEX_IS_DEPTH (PS_URBAN_CHAOS_HLE || PS_TALES_OF_ABYSS_HLE || PS_DEPTH_FMT == 1 || PS_DEPTH_FMT == 2)

#define SW_BLEND (PS_BLEND_A || PS_BLEND_B || PS_BLEND_D)
#define SW_BLEND_NEEDS_RT (SW_BLEND && (PS_BLEND_A == 1 || PS_BLEND_B == 1 || PS_BLEND_C == 1 || PS_BLEND_D == 1))
#define SW_AD_TO_HW (PS_BLEND_C == 1 && PS_A_MASKED)
#define NEEDS_RT_FOR_BLEND (((PS_BLEND_A != PS_BLEND_B) && (PS_BLEND_A == 1 || PS_BLEND_B == 1 || PS_BLEND_C == 1)) || PS_BLEND_D == 1 || SW_AD_TO_HW)
#define NEEDS_RT_EARLY (PS_TEX_IS_FB || PS_DATE >= 5)
#define NEEDS_RT_FOR_AFAIL (PS_AFAIL == AFAIL_ZB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY_SW_Z)
#define NEEDS_RT (NEEDS_RT_FOR_AFAIL || NEEDS_RT_EARLY || (!PS_PRIM_CHECKING_INIT && (PS_FBMASK || NEEDS_RT_FOR_BLEND)))
#define NEEDS_DEPTH_FOR_AFAIL (PS_AFAIL == AFAIL_FB_ONLY || PS_AFAIL == AFAIL_RGB_ONLY_SW_Z)
#define NEEDS_DEPTH_FOR_ZTST (PS_ZTST == ZTST_GEQUAL || PS_ZTST == ZTST_GREATER)
#define NEEDS_DEPTH_FOR_AA1 (PS_AA1 == PS_AA1_TRIANGLE_SW_Z)
#define SW_DEPTH (NEEDS_DEPTH_FOR_AFAIL || NEEDS_DEPTH_FOR_ZTST || NEEDS_DEPTH_FOR_AA1)
#define ZWRITE (PS_ZFLOOR || PS_ZCLAMP || SW_DEPTH)
#define NEED_PRIMID (PS_DATE >= 1 && PS_DATE <= 3)

#define PS_RETURN_COLOR_ROV (!PS_NO_COLOR && PS_ROV_COLOR)
#define PS_RETURN_COLOR (!PS_NO_COLOR && !PS_ROV_COLOR)
#define PS_RETURN_DEPTH_ROV (PS_ROV_DEPTH == PS_ROV_DEPTH_READ_WRITE)
#define PS_RETURN_DEPTH (ZWRITE && !PS_ROV_DEPTH)
#define PS_ROV_EARLYDEPTHSTENCIL (PS_ROV_COLOR && !PS_ROV_DEPTH && !ZWRITE)

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
#define PS_CHANNEL 0
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
#define PS_IIP 0
#define PS_ROUND_INV 0
#define PS_BLEND_MIX 0
#define PS_RTA_CORRECTION 0
#define PS_ZTST 0
#define PS_AA1 0
#define PS_ABE 0
#endif

struct PS_INPUT
{
	noperspective centroid float4 p : SV_Position;
	float4 t : TEXCOORD0;
	float4 ti : TEXCOORD2;
#if PS_IIP != 0
	float4 c : COLOR0;
#else
	nointerpolation float4 c : COLOR0;
#endif
	float inv_cov : COLOR1; // We use the inverse to make it simpler to interpolate.
	nointerpolation uint interior : COLOR2; // 1 for triangle interior; 0 for edge;
#if NEED_PRIMID
	uint prim_id : SV_PrimitiveID;
#endif
};

struct PS_OUTPUT
{
#define NUM_RTS 0

#if PS_RETURN_COLOR
	#if PS_DATE == 1 || PS_DATE == 2
		float c0 : SV_Target;
	#else
		
		float4 c0 : SV_Target0;

		#undef NUM_RTS
		#define NUM_RTS 1
		
		#if !PS_NO_COLOR1
			float4 c1 : SV_Target1;
		#endif
	#endif
#endif

#if PS_RETURN_DEPTH
	// In DX12 we do depth feedback loops with a color copy.
	#if SW_DEPTH && PS_NO_COLOR1 && PS_DEPTH_FEEDBACK_SUPPORT == 2
		#if NUM_RTS > 0
			float depth_color : SV_Target1;
		#else
			float depth_color : SV_Target0;
		#endif
	#endif
	#if PS_HAS_CONSERVATIVE_DEPTH && !SW_DEPTH
		float depth : SV_DepthLessEqual;
	#else
		float depth : SV_Depth;
	#endif
#endif

#undef NUM_RTS
};

Texture2D<float4> Texture : register(t0);
SamplerState TextureSampler : register(s0);
Texture2D<float4> Palette : register(t1);
Texture2D<float> PrimMinTexture : register(t3);
#if PS_ROV_COLOR
	RasterizerOrderedTexture2D<unorm float4> RtTextureRov : register(u0);
#else
	Texture2D<float4> RtTexture : register(t2);
#endif
#if PS_ROV_DEPTH
	RasterizerOrderedTexture2D<float> DepthTextureRov : register(u1);
#else
	Texture2D<float> DepthTexture : register(t4);
#endif

#ifdef DX12
cbuffer cb1 : register(b1)
#else
cbuffer cb1
#endif
{
  #define X(TYPE, NAME) TYPE NAME;
		PS_UNIFORMS(X)
	#undef X
};

PSInputGeneric GetPSInput(PS_INPUT psin)
{
  PSInputGeneric psin_gen;
  psin_gen.p = psin.p;
  psin_gen.t = psin.t;
  psin_gen.ti = psin.ti;
  psin_gen.c = psin.c;
  psin_gen.fc = psin.c;
  psin_gen.inv_cov = psin.inv_cov;
  psin_gen.interior = psin.interior;
  return psin_gen;
}

PSUniformsGeneric GetPSUniforms()
{
  PSUniformsGeneric cb;
	#define X(TYPE, NAME) cb.NAME = NAME;
		PS_UNIFORMS(X)
	#undef X
  return cb;
}

float4 RtLoad(int2 xy)
{
#if PS_ROV_COLOR
	return RtTextureRov[xy];
#else
	return RtTexture.Load(int3(int2(xy), 0));
#endif
}

float DepthLoad(int2 xy)
{
#if PS_ROV_DEPTH
	return DepthTextureRov[xy];
#else
	return DepthTexture.Load(int3(int2(xy), 0));
#endif
}

void RtWrite(int2 xy, float4 c)
{
#if PS_ROV_COLOR
	RtTextureRov[xy] = c;
#endif
}

void DepthWrite(int2 xy, float d)
{
#if PS_ROV_DEPTH
	DepthTextureRov[xy] = d;
#endif
}

#include "tfx_ps.inc"

#if PS_ROV_EARLYDEPTHSTENCIL
[earlydepthstencil]
#endif

#if (PS_RETURN_COLOR || PS_RETURN_DEPTH)
PS_OUTPUT ps_main(PS_INPUT input)
#else
void ps_main(PS_INPUT input)
#endif
{
	PSMainState state;
  state.psin = GetPSInput(input);
  state.cb = GetPSUniforms();
  state.tex = 0; // unused
  state.tex_depth = 0; // unused
  state.palette = 0; // unused
  state.prim_id_tex = 0; // unused
  state.tex_sampler = 0; // unused
	#if NEED_PRIMID
  	state.prim_id = input.prim_id;
	#else
		state.prim_id = 0;
	#endif
  state.color_discarded = false;
  state.depth_discarded = false;

	int2 coord = int2(state.psin.p.xy);

	state.current_depth = DepthLoad(coord);

	state.current_color = RtLoad(coord);

	#if (PS_RETURN_COLOR || PS_RETURN_DEPTH)
		PS_OUTPUT psout;
	#endif

	PSOutputGeneric psout_gen = ps_main_impl(state);

	// Color write back
	#if PS_RETURN_COLOR
		psout.c0 = psout_gen.c0;
		#if !PS_NO_COLOR1
			psout.c1 = psout_gen.c1;
		#endif
	#elif PS_RETURN_COLOR_ROV
		psout_gen.c0 = (FbMask == 0xFFu) ? state.current_color : psout_gen.c0; // channel masking
		if (!state.color_discarded)
			RtTextureRov[state.psin.p.xy] = psout_gen.c0;
	#endif

	// Depth write back
	#if PS_RETURN_DEPTH
		psout.depth = psout_gen.depth;
		#if SW_DEPTH && PS_NO_COLOR1 && PS_DEPTH_FEEDBACK_SUPPORT == 2
			// Output color clone for feedback.
			psout.depth_color = psout_gen.depth;
		#endif
	#elif PS_RETURN_DEPTH_ROV
		if (!state.depth_discarded)
			DepthTextureRov[state.psin.p.xy] = psout_gen.depth;
	#endif

	#if (PS_RETURN_COLOR || PS_RETURN_DEPTH)
		return psout;
	#endif
}

#endif // PIXEL_SHADER

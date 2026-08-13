// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#ifndef PCSX2_VULKAN
	#define PCSX2_VULKAN 0
#endif

#ifndef PCSX2_OPENGL
	#define PCSX2_OPENGL 0
#endif

#if PCSX2_VULKAN == PCSX2_OPENGL
	ERROR: Exactly one of PCSX2_VULKAN or PCSX2_OPENGL should be true.
#endif

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
#define UINT_BITCAST_UCHAR4(X) UINT4((X) & 0xFFu, ((X) >> 8) & 0xFFu, ((X) >> 16) & 0xFFu, ((X) >> 24) & 0xFFu)
#define MAT_MUL(X, Y) ((X) * (Y))
#define FRACT(X) fract(X)
#define MIX mix

#define VERTICES_PARAM(NAME) uint NAME
#define INDICES_PARAM(NAME) uint NAME
#define IN_PARAM(TYPE, NAME) TYPE NAME
#define IN_OUT_PARAM(TYPE, NAME) inout TYPE NAME

#define PRIMID_MAX 0x7FFFFFFF
#define VS_Y_FLIP 1.0f
#define EXP2_MIN_32 exp2(-32.0f)
#define EXP2_POS_32 exp2(32.0f)

#if PCSX2_VULKAN
	#define VS_SCALE_RAW_Z(Z) (float(Z) * EXP2_MIN_32)
#elif PCSX2_OPENGL
	#define VS_SCALE_RAW_Z(Z) ((HAS_CLIP_CONTROL != FALSE) ? (float(Z) * EXP2_MIN_32) : ((float(Z) * EXP2_MIN_32) * 2.0f - 1.0f))
#endif

#define PS_UV_MSK_FIX(CB) (FLOAT_BITCAST_UINT(CB.uv_min_max))
#define PS_SAMPLE_TEX(STATE, POS) (texture(Texture, FLOAT2(POS)))
#define PS_SAMPLE_TEX_LOD(STATE, POS, LOD) (textureLod(Texture, FLOAT2(POS), float(LOD)))
#define PS_SAMPLE_TEX_DEPTH(STATE, POS) (PS_SAMPLE_TEX(STATE, (POS)).r)
#define PS_SAMPLE_TEX_DEPTH_LOD(STATE, POS, LOD) (PS_SAMPLE_TEX_LOD(STATE, (POS), (LOD)).r)
#define PS_READ_TEX(STATE, POS, LOD) (texelFetch(Texture, INT2(POS), int(LOD)))
#define PS_READ_TEX_DEPTH(STATE, POS, LOD) (PS_READ_TEX(STATE, (POS), (LOD)).r)
#define PS_READ_PALETTE(STATE, POS) (texelFetch(Palette, INT2(POS), 0))
#define PS_READ_PRIMID(STATE, POS) (texelFetch(PrimMinTexture, INT2(POS), 0).r)
#define PS_GET_TEX_DIMS(STATE, OUT_VAR) (OUT_VAR = UINT2(textureSize(Texture, 0)))
#define PS_GET_TEX_DEPTH_DIMS(STATE, OUT_VAR) (PS_GET_TEX_DIMS(STATE, OUT_VAR))

#define LOAD_VERTEX(VERTICES, VID) load_vertex(VID)
#define LOAD_INDEX(INDICES, VID) load_index(VID)

#define FMT_32 0
#define FMT_24 1
#define FMT_16 2

#define SHUFFLE_READ  1
#define SHUFFLE_WRITE 2
#define SHUFFLE_READWRITE 3

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

#ifdef __METAL_VERSION__
  #define FALSE false
#else
  #define FALSE 0
#endif

struct VSUniformsGeneric
{
	FLOAT2 vertex_scale;
	FLOAT2 vertex_offset;
	FLOAT2 texture_scale;
	FLOAT2 texture_offset;
	FLOAT2 point_size;
	uint max_depth;
	float line_aa1_width;
};

struct VSInputGeneric
{
	FLOAT2 st;
	FLOAT4 c;
	float  q;
	UINT2  p;
	uint   z;
	UINT2  uv;
	FLOAT4 f;
};

struct VSOutputGeneric
{
	FLOAT4 p;
  FLOAT4 t;
	FLOAT4 ti;
  FLOAT4 c;
	float inv_cov;
	uint interior;
	float point_size;
};

struct PSUniformsGeneric
{
  FLOAT3 fog_color;
  float aref;
	FLOAT4 wh; ///< xy => PS2, zw => actual (upscaled)
	FLOAT2 ta;
	float max_depth;
	float alpha_fix;
	UINT4 fbmask;

	FLOAT4 half_texel;
  FLOAT4 uv_min_max;
	FLOAT4 lod_params;
	FLOAT4 st_range;
  
  uint channel_shuffle_blue_mask;
  uint channel_shuffle_blue_shift;
  uint channel_shuffle_green_mask;
  uint channel_shuffle_green_shift;

	FLOAT2 channel_shuffle_offset;
	FLOAT2 tc_offset;
	FLOAT2 st_scale;
	FLOAT4x4 dither_matrix;

	FLOAT4 scale_factor;

	float line_cov_scale;
	float _pad0;
	float _pad1;
	float _pad2;
};

struct PSInputGeneric
{
	FLOAT4 p;
	FLOAT4 t;
	FLOAT4 ti;
	FLOAT4 c;
	FLOAT4 fc;
	float inv_cov;
	uint interior;
};

// Main state of the pixel shader invocation.
#ifdef __METAL_VERSION__
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
    const thread MainPSIn& psinput;
    constant GSMTLMainPSUniform& cb;

    PSMain(const thread MainPSIn& psinput, constant GSMTLMainPSUniform& cb): psinput(psinput), cb(cb) {}
  };
#else
  struct PSMain
  {
    uint tex; // unused
    uint tex_depth; // unused
    uint palette; // unused
    uint prim_id_tex; // unused
    uint tex_sampler; // unused
    FLOAT4 current_color;
    float current_depth;
    uint prim_id;
    bool color_discarded;
    bool depth_discarded;
    PSInputGeneric psinput;
    PSUniformsGeneric cb;
  };
#endif

struct PSOutputGeneric
{
	FLOAT4 c0;
	FLOAT4 c1;
	float depth;
};

#ifndef __METAL_VERSION__
STATIC FLOAT4 convert_depth32_rgba8(float value)
{
	uint val = uint(value * EXP2_POS_32);
	return FLOAT4(UINT_BITCAST_UCHAR4(val));
}

STATIC FLOAT4 convert_depth16_rgba8(float value)
{
	uint val = uint(value * EXP2_POS_32);
	return FLOAT4(UINT4(val << 3, val >> 2, val >> 7, val >> 8) & UINT4(0xf8, 0xf8, 0xf8, 0x80));
}
#endif

STATIC FLOAT2 float2_bcast(float val)
{
  return FLOAT2(val, val);
}

STATIC FLOAT3 float3_bcast(float val)
{
  return FLOAT3(val, val, val);
}

STATIC FLOAT4 float4_bcast(float val)
{
  return FLOAT4(val, val, val, val);
}


//////////////////////////////////////////////////////////////////////
// Vertex Shader
//////////////////////////////////////////////////////////////////////

#ifdef VERTEX_SHADER

#ifndef VS_TME
#define VS_TME 0
#define VS_FST 0
#define VS_IIP 0
#define VS_POINT_SIZE 0
#define VS_EXPAND_TYPE VS_EXPAND_NONE
#endif

#if PCSX2_VULKAN
	layout(push_constant) uniform cb2
#elif PCSX2_OPENGL
	layout(std140, binding = 4) uniform cb22
#endif
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

#if PCSX2_VULKAN
	layout(std140, set = 0, binding = 2)
#elif PCSX2_OPENGL
	layout(std140, binding = 2)
#endif
readonly buffer VertexBuffer
{
	RawVertex vertex_buffer[];
};

// Warning: use std430 instead of std140 so that the ints are tightly packed.
#if PCSX2_VULKAN
	layout(std430, set = 0, binding = 3) 
#elif PCSX2_OPENGL
	layout(std430, binding = 3)
#endif
readonly buffer IndexBuffer
{
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
STATIC VSOutputGeneric vs_main_impl(IN_PARAM(VSInputGeneric, v), IN_PARAM(VSUniformsGeneric, cb))
{
	VSOutputGeneric vout;
	// Clamp to max depth, gs doesn't wrap
	uint z = min(v.z, cb.max_depth);
	vout.p.xy = FLOAT2(v.p) - FLOAT2(0.05f, 0.05f);
	vout.p.xy = vout.p.xy * cb.vertex_scale - cb.vertex_offset;
  vout.p.y *= VS_Y_FLIP;
	vout.p.w = 1.0f;
	vout.p.z = VS_SCALE_RAW_Z(z);

  FLOAT2 uv = FLOAT2(v.uv) - cb.texture_offset;
	FLOAT2 st = v.st - cb.texture_offset;

	// Float coordinate
	vout.t.xy = st;
	vout.t.w = v.q;

	// Integer coordinate => normalized
	vout.ti.xy = uv * cb.texture_scale;

	if (VS_FST != FALSE)
	{
		// Integer coordinate => integral
		vout.ti.zw = uv;
	}
	else
	{
		// Some games uses float coordinate for post-processing effects
		vout.ti.zw = st / cb.texture_scale;
	}

  vout.c = v.c;

	vout.t.z = v.f.x; // pack fog with texture

	if (VS_POINT_SIZE != FALSE)
		vout.point_size = cb.point_size.x;

	return vout;
}

// Convert XY from NDC to GS pixel coordinates (i.e. 1.0 = 1 GS pixel).
STATIC FLOAT2 get_xy_unscaled(FLOAT2 xy, IN_PARAM(VSUniformsGeneric, cb))
{
	return round(xy / cb.vertex_scale) / 16.0f;
}

// Get the XY deltas in GS pixel coordinates, using first vertex as the origin.
STATIC FLOAT2x2 get_xy_deltas_unscaled(IN_PARAM(VSOutputGeneric, v0), IN_PARAM(VSOutputGeneric, v1), IN_PARAM(VSOutputGeneric, v2), IN_PARAM(VSUniformsGeneric, cb))
{
	FLOAT2 xy0 = get_xy_unscaled(v0.p.xy, cb);
	FLOAT2 xy1 = get_xy_unscaled(v1.p.xy, cb);
	FLOAT2 xy2 = get_xy_unscaled(v2.p.xy, cb);
	return FLOAT2x2(xy1 - xy0, xy2 - xy0);
}

// Get the AA1 outward expand direction to the edge formed by the first two vertices.
// This is up or down for shallow (X dominant) edges, and right or left for steep (Y dominant) edges.
// Similar expansion to line AA1 except instead of expanding on both sides of the line,
// expand on on the side towards the outside of the triangle.
STATIC FLOAT2 get_aa1_triangle_expand_dir(IN_PARAM(VSOutputGeneric, v0), IN_PARAM(VSOutputGeneric, v1), IN_PARAM(VSOutputGeneric, v2), IN_PARAM(VSUniformsGeneric, cb))
{
	FLOAT2x2 xy_deltas = get_xy_deltas_unscaled(v0, v1, v2, cb);
	FLOAT2 line_delta = xy_deltas[0];
	FLOAT2 line_opposite = xy_deltas[1];

	FLOAT2 line_normal = FLOAT2(line_delta.y, -line_delta.x);
	FLOAT2 line_expand = abs(line_delta.x) >= abs(line_delta.y) ? FLOAT2(0.0f, 1.0f) : FLOAT2(1.0f, 0.0f);

	if ((dot(line_expand, line_normal) >= 0.0f) == (dot(line_opposite, line_normal) >= 0.0f))
	{
		// Expand direction point towards the interior so flip it.
		line_expand = -line_expand;
	}

	return line_expand;
}

// This works for GLSL, HLSL, MSL in spite of different row/column ordering.
STATIC FLOAT2x2 get_inverse(IN_PARAM(FLOAT2x2, mat), float det)
{
	return FLOAT2x2(mat[1][1], -mat[0][1], -mat[1][0], mat[0][0]) * (1 / det);
}

// Extrapolate triangle attributes from the first vertex along the given direction.
// dp_mat is derived from the input vertices, it is passed in to avoid recomputing.
STATIC void extrapolate_aa1_triangle_edge(IN_OUT_PARAM(VSOutputGeneric, v0), IN_PARAM(VSOutputGeneric, v1), IN_PARAM(VSOutputGeneric, v2),
	IN_PARAM(FLOAT2x2, dp_mat), FLOAT2 dp, IN_PARAM(VSUniformsGeneric, cb))
{
	// Get texture deltas
	FLOAT2x2 dt;
	if (VS_FST != FALSE)
	{
		dt = FLOAT2x2(v1.ti.zw - v0.ti.zw, v2.ti.zw - v0.ti.zw);
	}
	else
	{
		dt = FLOAT2x2(v1.t.xy - v0.t.xy, v2.t.xy - v0.t.xy);
	}

	// Get color delta if interpolating
	FLOAT2x4 dc;
	if (VS_IIP != FALSE)
	{
		dc = FLOAT2x4(v1.c - v0.c, v2.c - v0.c);
	}

	FLOAT2 dz = FLOAT2(v1.p.z - v0.p.z, v2.p.z - v0.p.z); // Z deltas

	FLOAT2 df = FLOAT2(v1.t.z - v0.t.z, v2.t.z - v0.t.z); // Fog deltas

	FLOAT2 dq = FLOAT2(v1.t.w - v0.t.w, v2.t.w - v0.t.w); // Q deltas

	// To prevent unstable extrapolation, do not extrapolate if the
	// minimum perpendicular length of the triangle is < 2 pixels.
	float dp_det = determinant(dp_mat); // Twice signed triangle area.
	float len0 = length(dp_mat[0]);
	float len1 = length(dp_mat[1]);
	float len2 = length(dp_mat[1] - dp_mat[0]);
	float min_perp_length = abs(dp_det) / max(max(len0, len1), len2);

	// Get the position -> barycentric weight matrix
	FLOAT2x2 inv_dp_mat = get_inverse(dp_mat, dp_det);

	FLOAT2 weights = min_perp_length < 2 ? FLOAT2(0.0f, 0.0f) : MAT_MUL(inv_dp_mat, dp);

	v0.p.xy += dp * cb.point_size; // Extrapolate position

	// Extrapolate texture coords
	if (VS_FST != FALSE)
	{
			v0.ti.zw += MAT_MUL(dt, weights);
			v0.ti.xy = v0.ti.zw * cb.texture_scale;
	}
	else
	{
		v0.t.xy += MAT_MUL(dt, weights);
		v0.ti.zw = v0.t.xy / cb.texture_scale;
		v0.t.w += dot(dq, weights);
	}

	// Extrapolate and clamp color
	if (VS_IIP != FALSE)
	{
		v0.c += MAT_MUL(dc, weights);
		v0.c = clamp(v0.c, 0, 255);
	}

	v0.p.z += dot(dz, weights); // Extrapolate depth

	v0.t.z += dot(df, weights); // Extrapolate fog
}

STATIC VSOutputGeneric vs_expand_none_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  return vs_main_impl(LOAD_VERTEX(vertices, vid), cb);
}

STATIC VSOutputGeneric vs_expand_point_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  VSOutputGeneric vout = vs_main_impl(LOAD_VERTEX(vertices, vid), cb);
  if ((vid & 1u) != 0u)
    vout.p.x += cb.point_size.x;
  if ((vid & 2u) != 0u)
    vout.p.y += cb.point_size.y;
  return vout;
}

STATIC VSOutputGeneric vs_expand_line_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  uint vid_base = vid >> 2;
  bool is_bottom = (vid & 2u) != 0u;
  bool is_right = (vid & 1u) != 0u;
  uint vid_other = is_bottom ? vid_base - 1 : vid_base + 1;
  VSOutputGeneric vout = vs_main_impl(LOAD_VERTEX(vertices, vid_base), cb);
  VSOutputGeneric other = vs_main_impl(LOAD_VERTEX(vertices, vid_other), cb);

  // Use bottom minus top for delta regardless of which vertex we are expanding.
  FLOAT2 line_delta = is_bottom ? vout.p.xy - other.p.xy : other.p.xy - vout.p.xy;
  FLOAT2 line_vector = normalize(line_delta / cb.vertex_scale);
  FLOAT2 line_expand = FLOAT2(line_vector.y, -line_vector.x);

  if (VS_EXPAND_TYPE == VS_EXPAND_LINE_AA1)
    line_expand *= 2.f * cb.line_aa1_width;

  FLOAT2 line_width = (line_expand * cb.point_size) / 2;
  FLOAT2 offset = is_right ? line_width : -line_width;
  vout.p.xy += offset;

  if (VS_EXPAND_TYPE == VS_EXPAND_LINE_AA1)
    vout.inv_cov = is_right ? 1.f : -1.f;

  // Lines will be run as (0 1 2) (1 2 3)
  // This means that both triangles will have a point based off the top line point as their first point
  // So we don't have to do anything for !IIP

  return vout;
}

STATIC VSOutputGeneric vs_expand_sprite_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  uint vid_base = vid >> 1;
  bool is_bottom = (vid & 2u) != 0u;
  bool is_right = (vid & 1u) != 0u;
  // Sprite points are always in pairs
  uint vid_lt = vid_base & ~1u;
  uint vid_rb = vid_base | 1u;

  VSOutputGeneric lt = vs_main_impl(LOAD_VERTEX(vertices, vid_lt), cb);
  VSOutputGeneric rb = vs_main_impl(LOAD_VERTEX(vertices, vid_rb), cb);
  VSOutputGeneric vout = rb;

  if (!is_right)
  {
    vout.p.x = lt.p.x;
    vout.t.x = lt.t.x;
    vout.ti.xz = lt.ti.xz;
  }

  if (!is_bottom)
  {
    vout.p.y = lt.p.y;
    vout.t.y = lt.t.y;
    vout.ti.yw = lt.ti.yw;
  }

  return vout;
}

STATIC VSOutputGeneric vs_expand_triangle_aa1_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb), INDICES_PARAM(indices))
{
  // Triangles with AA1 are expanded as follows:
  // - Vertices 0-2: Interior of triangle (1 triangle).
  // - Vertices 3-8: First edge expanded (2 triangles).
  // - Vertices 9-14: Second edge expanded (2 triangles).
  // - Vertices 15-20: Third edge expanded (2 triangles).
  // - Vertices 21-26: First corner cap (2 triangles).
  // - Vertices 27-32: Second corner cap (2 triangles).
  // - Vertices 33-38: Third corner cap (2 triangles).

  uint prim_id = vid / 39;
  uint prim_offset = vid - 39 * prim_id; // range: 0-38
  bool interior = prim_offset < 3;
  bool edge = 3 <= prim_offset && prim_offset < 21;

  VSOutputGeneric vout;
  if (interior)
  {
    vout = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + prim_offset)), cb);
    vout.inv_cov = 0.f;
    vout.interior = 1;
  }
  else if (edge)
  {
    // Vertex indices for this edge. We need all 3 for determining exterior/interior.
    uint prim_offset_edges = prim_offset - 3; // range: 0-17
    uint i0 = prim_offset_edges / 6;
    uint i1 = (i0 >= 2) ? i0 - 2 : i0 + 1;
    uint i2 = (i0 >= 1) ? i0 - 1 : i0 + 2;
    uint edge_offset = prim_offset_edges - 6 * i0; // range: 0-5

    // Note: order of top/bottom, inside/outside is arbitrary,
    // as long as it assembles into two triangles forming a quad.
    bool is_bottom = (2 <= edge_offset) && (edge_offset <= 4);
    bool is_outside = (edge_offset & 1u) != 0u;

    vout                     = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + (is_bottom ? i1 : i0))), cb);
    VSOutputGeneric other    = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + (is_bottom ? i0 : i1))), cb);
    VSOutputGeneric opposite = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + i2)), cb);

    FLOAT2x2 pos_deltas = get_xy_deltas_unscaled(vout, other, opposite, cb);

    FLOAT2 expand_dir = is_outside ? get_aa1_triangle_expand_dir(vout, other, opposite, cb) : FLOAT2(0.0f, 0.0f);

    // Do actual extrapolation, or no-op if expand_dir == 0.
    extrapolate_aa1_triangle_edge(vout, other, opposite, pos_deltas, expand_dir, cb);

    vout.inv_cov = is_outside ? 1.0f : 0.0f; // No coverage on outside, otherwise full.

    vout.interior = 0;
  }
  else // Corner cap
  {
    // Vertex indices for this cap. We need all 3 for determining exterior/interior.
    uint prim_offset_cap = prim_offset - 21; // range: 0-8
    uint i0 = prim_offset_cap / 6;
    uint i1 = (i0 >= 2) ? i0 - 2 : i0 + 1;
    uint i2 = (i0 >= 1) ? i0 - 1 : i0 + 2;
    uint cap_offset = prim_offset_cap - 6 * i0; // range: 0-5

    bool is_near_corner = cap_offset == 0 || cap_offset == 3;
    bool is_far_corner = cap_offset == 2 || cap_offset == 5;
    bool is_first_tri = cap_offset < 3;

    vout                     = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + i0)), cb);
    VSOutputGeneric other    = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + (is_first_tri ? i1 : i2))), cb);
    VSOutputGeneric opposite = vs_main_impl(LOAD_VERTEX(vertices, LOAD_INDEX(indices, 3 * prim_id + (is_first_tri ? i2 : i1))), cb);

    FLOAT2x2 pos_deltas = get_xy_deltas_unscaled(vout, other, opposite, cb);

    // Get the edge expansion directions of both incident edges.
    FLOAT2 edge_expand_dir_0 = get_aa1_triangle_expand_dir(vout, other, opposite, cb);
    FLOAT2 edge_expand_dir_1 = get_aa1_triangle_expand_dir(vout, opposite, other, cb);

    // Check if the corner is already filled by the expanded edges.
    // This happens if the expand directions are the same.
    // If so we output a degenerate triangle at this corner.
    bool corner_filled = all(VEQUAL(edge_expand_dir_0, edge_expand_dir_1));

    // Nothing if corner is filled, otherwise opposite to the bisector of the corner angle.
    FLOAT2 far_corner_dir = corner_filled ? FLOAT2(0.0f, 0.0f) : -normalize((pos_deltas[0] + pos_deltas[1]) / 2);

    // Determine the expand direction.
    FLOAT2 expand_dir = is_near_corner ? FLOAT2(0.0f, 0.0f) : // No extrapolation
                        is_far_corner ? far_corner_dir :      // Opposite to the angle bisector of corner
                        edge_expand_dir_0;                    // Standard AA1 edge expansion

    // Do the actual extrapolation (no-op if expand_dir == 0).
    extrapolate_aa1_triangle_edge(vout, other, opposite, pos_deltas, expand_dir, cb);

    vout.inv_cov = is_near_corner ? 0.0f : 1.0f; // Full coverage at near corner, otherwise none.
  
    vout.interior = 0;
  }

  return vout;
}

STATIC VSOutputGeneric vs_expand_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb), INDICES_PARAM(indices))
{
	switch (VS_EXPAND_TYPE)
	{
		case VS_EXPAND_NONE:
			return vs_expand_none_impl(vid, vertices, cb);
		case VS_EXPAND_POINT:
      return vs_expand_point_impl(vid, vertices, cb);
		case VS_EXPAND_LINE:
		case VS_EXPAND_LINE_AA1:
      return vs_expand_line_impl(vid, vertices, cb);
		case VS_EXPAND_SPRITE:
      return vs_expand_sprite_impl(vid, vertices, cb);
		case VS_EXPAND_TRIANGLE_AA1:
      return vs_expand_triangle_aa1_impl(vid, vertices, cb, indices);
	}
}



#if PCSX2_VULKAN
	layout(std140, set = 0, binding = 0) uniform cb0
#elif PCSX2_OPENGL
	layout(std140, binding = 1) uniform cb20
#endif
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

#if PCSX2_VULKAN
	layout(location = 0) out VSOutput
#elif PCSX2_OPENGL
	out SHADER
#endif
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
	#if VS_POINT_SIZE
  	gl_PointSize = v.point_size;
	#endif
}

#if VS_EXPAND_TYPE == VS_EXPAND_NONE

// Note: VK and GL have different layouts as GL uses the same layout
// for convert shaders and TFX shaders, and thus has more attributes.
#if PCSX2_VULKAN
	layout(location = 0) in vec2  a_st;
	layout(location = 1) in uvec4 a_c;
	layout(location = 2) in float a_q;
	layout(location = 3) in uvec2 a_p;
	layout(location = 4) in uint  a_z;
	layout(location = 5) in uvec2 a_uv;
	layout(location = 6) in vec4  a_f;
#elif PCSX2_OPENGL
	layout(location = 0) in vec2  a_st;
	layout(location = 2) in vec4  a_c;
	layout(location = 3) in float a_q;
	layout(location = 4) in uvec2 a_p;
	layout(location = 5) in uint  a_z;
	layout(location = 6) in uvec2 a_uv;
	layout(location = 7) in vec4  a_f;
#endif

VSInputGeneric GetVSInput()
{
  VSInputGeneric vin;
  vin.st = a_st;
  vin.c = FLOAT4(a_c);
  vin.q = a_q;
  vin.p = a_p;
  vin.z = a_z;
  vin.uv = a_uv;
  vin.f = a_f;
  return vin;
}

void main()
{
  VSInputGeneric vin = GetVSInput();
  VSUniformsGeneric cb = GetVSUniforms();
	VSOutputGeneric vout = vs_main_impl(vin, cb);
  WriteVSOutput(vout);
}

#else // VS_EXPAND_TYPE

void main()
{
	#if PCSX2_VULKAN
		uint vid = uint(gl_VertexIndex);
	#elif PCSX2_OPENGL
		uint vid = uint(gl_VertexID);
	#endif
  VSUniformsGeneric cb = GetVSUniforms();
  VSOutputGeneric vout = vs_expand_impl(vid, 0, cb, 0);
  WriteVSOutput(vout);
}

#endif // VS_EXPAND_TYPE

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

#if PCSX2_VULKAN
	layout(std140, set = 0, binding = 1) uniform cb1
#elif PCSX2_OPENGL
	layout(std140, binding = 0) uniform cb21
#endif
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

#if PCSX2_VULKAN
	layout(location = 0) in VSOutput
#elif PCSX2_OPENGL
	in SHADER
#endif
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

#define TARGET_0_QUALIFIER out

// Only enable framebuffer fetch when we actually need it.
#if PCSX2_OPENGL && HAS_FRAMEBUFFER_FETCH && NEEDS_RT
	// We need to force the colour to be defined here, to read from it.
	// Basically the only scenario where this'll happen is RGBA masked and DATE is active.
	#undef PS_NO_COLOR
	#define PS_NO_COLOR 0
	#if defined(GL_EXT_shader_framebuffer_fetch)
		#undef TARGET_0_QUALIFIER
		#define TARGET_0_QUALIFIER inout
		#define LAST_FRAG_COLOR o_col0
	#elif defined(GL_ARM_shader_framebuffer_fetch)
		#define LAST_FRAG_COLOR gl_LastFragColorARM
	#endif
#endif

#if PS_RETURN_COLOR
	#if !PS_NO_COLOR1
		layout(location = 0, index = 0) TARGET_0_QUALIFIER vec4 o_col0;
		layout(location = 0, index = 1) out vec4 o_col1;
	#elif !PS_NO_COLOR
		layout(location = 0) out vec4 o_col0;
	#endif
#endif

// Depth feedback mode 2 is for depth as color.
// Use FB fetch for the feedback if it's available.
#if PCSX2_OPENGL && PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && PS_NO_COLOR1 && (DEPTH_FEEDBACK_SUPPORT == 2)
	#if HAS_FRAMEBUFFER_FETCH
		layout(location = 1) inout float o_col1;
	#else
		layout(location = 1) out float o_col1;
	#endif
#endif

#if PCSX2_VULKAN
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
#elif PCSX2_OPENGL
	layout(binding = 0) uniform sampler2D Texture;
	layout(binding = 1) uniform sampler2D Palette;
	layout(binding = 2) uniform sampler2D RtSampler;
	layout(binding = 3) uniform sampler2D PrimMinTexture;
	layout(binding = 4) uniform sampler2D DepthSampler;
#endif

#if ZWRITE && PS_HAS_CONSERVATIVE_DEPTH && !PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH
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
#if PCSX2_VULKAN
	#if PS_ROV_COLOR
		return imageLoad(RtImageRov, xy);
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_RT && USE_FEEDBACK_SAMPLER
		return texelFetch(RtSampler, xy, 0);
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_RT
		return subpassLoad(RtSampler);
	#else
		return vec4(0.0f);
	#endif
#elif PCSX2_OPENGL
	#if !PS_FEEDBACK_LOOP_IS_NEEDED_RT
		return vec4(0.0f);
	#elif HAS_FRAMEBUFFER_FETCH
		return LAST_FRAG_COLOR;
	#else
		return texelFetch(RtSampler, xy, 0);
	#endif
#endif
}

float DepthLoad(ivec2 xy)
{
#if PCSX2_VULKAN
	#if PS_ROV_COLOR
		return imageLoad(DepthImageRov, xy).r;
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && USE_FEEDBACK_SAMPLER
		return texelFetch(DepthSampler, xy, 0).r;
	#elif PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH
		return subpassLoad(DepthSampler).r;
	#else
		return 0.0f;
	#endif
#elif PCSX2_OPENGL
	#if !PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH
		return 0.0f;
	#elif HAS_FRAMEBUFFER_FETCH && (DEPTH_FEEDBACK_SUPPORT == 2)
		return o_col1;
	#else
		return texelFetch(DepthSampler, xy, 0).r;
	#endif
#endif
}

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

// Metal needs to distinguish depth texture from color texture, other APIs don't.
#ifndef __METAL_VERSION__
	#define PS_TEX_IS_DEPTH 0
#endif

STATIC void discard_all(IN_OUT_PARAM(PSMain, state))
{
  if (PS_ROV_COLOR != FALSE || PS_ROV_DEPTH != PS_ROV_DEPTH_NONE)
    state.color_discarded = state.depth_discarded = true;
  else
    GPU_DISCARD;
}

STATIC void discard_color(IN_OUT_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, color))
{
  if (PS_ROV_COLOR != FALSE)
    state.color_discarded = true;
  else
    color = state.current_color;
}

STATIC void discard_depth(IN_OUT_PARAM(PSMain, state), IN_OUT_PARAM(float, depth))
{
  if (PS_ROV_DEPTH == PS_ROV_DEPTH_READ_WRITE)
    state.depth_discarded = true;
  else
    depth = state.current_depth;
}

FLOAT4 sample_tex_lod(IN_PARAM(PSMain, state), FLOAT2 uv, float lod)
{
  if (PS_TEX_IS_DEPTH != FALSE)
    return FLOAT4(PS_SAMPLE_TEX_DEPTH_LOD(state, uv, lod), 0.0f, 0.0f, 0.0f);
  else
    return PS_SAMPLE_TEX_LOD(state, uv, lod);
}

FLOAT4 sample_tex(IN_PARAM(PSMain, state), FLOAT2 uv)
{
  if (PS_TEX_IS_DEPTH != FALSE)
    return FLOAT4(PS_SAMPLE_TEX_DEPTH(state, uv), 0.0f, 0.0f, 0.0);
  else
    return PS_SAMPLE_TEX(state, uv);
}

FLOAT4 read_tex(IN_PARAM(PSMain, state), UINT2 pos, uint lod)
{
  if (PS_TEX_IS_DEPTH != FALSE)
    return FLOAT4(PS_READ_TEX_DEPTH(state, pos, lod), 0.0f, 0.0f, 0.0f);
  else
    return PS_READ_TEX(state, pos, lod);
}

FLOAT4 read_tex(IN_PARAM(PSMain, state), UINT2 pos)
{
  return read_tex(state, pos, 0);
}

UINT2 get_tex_dims(IN_PARAM(PSMain, state))
{
  UINT2 dims = UINT2(0, 0);
  if (PS_TEX_IS_DEPTH != FALSE)
    PS_GET_TEX_DEPTH_DIMS(state, dims);
  else
    PS_GET_TEX_DIMS(state, dims);
  return dims;
}

STATIC float manual_lod(IN_PARAM(PSMain, state), float uv_w)
{
  // FIXME add LOD: K - ( LOG2(Q) * (1 << L))
  float K = state.cb.lod_params.x;
  float L = state.cb.lod_params.y;
  float bias = state.cb.lod_params.z;
  float max_lod = state.cb.lod_params.w;

  float gs_lod = K - log2(abs(uv_w)) * L;
  // FIXME max useful ?
  //return max(min(gs_lod, max_lod) - bias, 0.0f);
  return min(gs_lod, max_lod) - bias;
}

STATIC FLOAT4 sample_c_af(IN_OUT_PARAM(PSMain, state), FLOAT2 uv, float uv_w)
{
  // HW sampler will reject bad UVs, match that here.
  uv = any(BOOL2(INT2(isnan(uv)) | INT2(isinf(uv)))) ? FLOAT2(0.0f, 0.0f) : uv;

  // Large floating point values risk NaN/Inf values.
  // Above this value floats lose decimal precision, so seems a resonable limit for UVs.
  uv = clamp(uv, -8388608.0f, 8388608.0f);

  // Below taken from https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#7.18.11%20LOD%20Calculations
  // And https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
  // With guidance from https://pema.dev/2025/05/09/mipmaps-too-much-detail/
  FLOAT2 sz = FLOAT2(get_tex_dims(state));
  FLOAT2 dX = DFDX(uv) * sz;
  FLOAT2 dY = DFDY(uv) * sz;

  float length_x = length(dX);
  float length_y = length(dY);

  // Calculate Ellipse Transform
  bool d_zero = length_x < 0.001f || length_y < 0.001f;
  float f = (dX.x * dY.y - dX.y * dY.x);
  bool d_par = f < 0.001f;
  bool d_per = dot(dX, dY) < 0.001f;
  bool d_inf_nan = any(BOOL2(INT2(isinf(dX)) | INT2(isinf(dY)) | INT2(isnan(dX)) | INT2(isnan(dY))));

  if (!(d_zero || d_par || d_per || d_inf_nan))
  {
    float A = dX.y * dX.y + dY.y * dY.y;
    float B = -2 * (dX.x * dX.y + dY.x * dY.y);
    float C = dX.x * dX.x + dY.x * dY.x;
    float F = f * f;

    float p = A - C;
    float q = A + C;
    float t = sqrt(p * p + B * B);

    float signB = sign(B);
    float denom_plus  = t * (q + t);
    float denom_minus = t * (q - t);

    float sqrtA = sqrt(F * (t + p));
    float sqrtB = sqrt(F * (t - p));

    float inv_sqrt_denom_plus  = RSQRT(denom_plus);
    float inv_sqrt_denom_minus = RSQRT(denom_minus);

    FLOAT2 new_dX = FLOAT2(
      sqrtA * inv_sqrt_denom_plus,
      sqrtB * inv_sqrt_denom_plus * signB
    );

    FLOAT2 new_dY = FLOAT2(
      sqrtB * inv_sqrt_denom_minus * -signB,
      sqrtA * inv_sqrt_denom_minus
    );

    d_inf_nan = any(BOOL2(INT2(isinf(new_dX)) | INT2(isinf(new_dY)) | INT2(isnan(new_dX)) | INT2(isnan(new_dY))));
    if (!d_inf_nan)
    {
      dX = new_dX;
      dY = new_dY;
      length_x = length(dX);
      length_y = length(dY);
    }
  }

  // Compute AF values
  bool is_major_x = length_x > length_y;
  float length_major = is_major_x ? length_x : length_y;
  float length_minor = is_major_x ? length_y : length_x;

  float aniso_ratio;
  float length_lod;
  FLOAT2 aniso_line;
  if (length_major <= 1.0f)
  {
    // A zero length_major would result in NaN Lod and break sampling.
    // A small length_major would result in aniso_ratio getting clamped to 1.
    // Perform isotropic filtering instead.
    aniso_ratio = 1.0f;
    length_lod = length_major;
    aniso_line = FLOAT2(0.0f, 0.0f);
  }
  else
  {
    FLOAT2 aniso_line_dir = is_major_x ? dX : dY;

    aniso_ratio = min(length_major / length_minor, float(PS_SW_ANISO));
    length_lod = length_major / aniso_ratio;

    // clamp to top Lod
    if (length_lod < 1.0f)
      aniso_ratio = max(1.0f, aniso_ratio * length_lod);

    aniso_ratio = round(aniso_ratio);

    aniso_line = aniso_line_dir * 0.5f * (1.0f / sz);
  }

  float lod = PS_AUTOMATIC_LOD != FALSE ? log2(length_lod) : PS_MANUAL_LOD != FALSE ? manual_lod(state, uv_w) : 0.0f;

  FLOAT4 colour;
  if (aniso_ratio == 1.0f)
  {
    colour = sample_tex_lod(state, uv, LEVEL(lod));
  }
  else
  {
    FLOAT4 num = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    FLOAT2 segment = (2.0f * aniso_line) / aniso_ratio;
    for (int i = 0; i < aniso_ratio; i++)
    {
      FLOAT2 d = -aniso_line + (0.5f + i) * segment;
      FLOAT2 uv_sample = uv + d;
      FLOAT4 sample_colour = sample_tex_lod(state, uv_sample, LEVEL(lod));
      num += sample_colour;
    }

    colour = num / aniso_ratio;
  }
  return colour;
}

STATIC FLOAT4 sample_c(IN_PARAM(PSMain, state), FLOAT2 uv)
{
  if (PS_TEX_IS_FB != 0)
    return state.current_color;
  if (PS_REGION_RECT != 0)
    return read_tex(state, UINT2(uv));

  if (PS_ADJS == FALSE && PS_ADJT == FALSE)
  {
    uv *= state.cb.st_scale;
  }
  else
  {
    if (PS_ADJS != FALSE)
      uv.x = (uv.x - state.cb.st_range.x) * state.cb.st_range.z;
    else
      uv.x = uv.x * state.cb.st_scale.x;
    if (PS_ADJT != FALSE)
      uv.y = (uv.y - state.cb.st_range.y) * state.cb.st_range.w;
    else
      uv.y = uv.y * state.cb.st_scale.y;
  }

  if (PS_SW_ANISO > 1)
    return sample_c_af(state, uv, state.psinput.t.w);
  else if (PS_AUTOMATIC_LOD != FALSE)
    return sample_tex(state, uv);
  else if (PS_MANUAL_LOD != FALSE)
    return sample_tex_lod(state, uv, LEVEL(manual_lod(state, state.psinput.t.w)));
  else
    return sample_tex_lod(state, uv, LEVEL(0));
}

FLOAT4 sample_p(IN_PARAM(PSMain, state), uint idx)
{
  return PS_READ_PALETTE(state, UINT2(idx, 0));
}

FLOAT4 sample_p_norm(IN_PARAM(PSMain, state), float u)
{
  return sample_p(state, uint(u * 255.5f));
}

FLOAT4 clamp_wrap_uv(IN_PARAM(PSMain, state), FLOAT4 uv)
{
  FLOAT4 tex_size = state.cb.wh.xyxy;

  if (PS_WMS == PS_WMT)
  {
    if (PS_REGION_RECT != FALSE && PS_WMS == 0)
    {
      uv = FRACT(uv);
    }
    else if (PS_REGION_RECT != FALSE && PS_WMS == 1)
    {
      uv = SATURATE(uv);
    }
    else if (PS_WMS == 2)
    {
      uv = clamp(uv, state.cb.uv_min_max.xyxy, state.cb.uv_min_max.zwzw);
    }
    else if (PS_WMS == 3)
    {
      // wrap negative uv coords to avoid an off by one error that shifted
      // textures. Fixes Xenosaga's hair issue.
      if (PS_FST == FALSE)
        uv = FRACT(uv);

      UINT4 uv_msk_fix = PS_UV_MSK_FIX(state.cb);
      uv = FLOAT4((USHORT4(uv * tex_size) & USHORT4(uv_msk_fix.xyxy)) | USHORT4(uv_msk_fix.zwzw)) / tex_size;
    }
  }
  else
  {
    if (PS_REGION_RECT != FALSE && PS_WMS == 0)
    {
      uv.xz = FRACT(uv.xz);
    }
    else if (PS_REGION_RECT != FALSE && PS_WMS == 1)
    {
      uv.xz = SATURATE(uv.xz);
    }
    else if (PS_WMS == 2)
    {
      uv.xz = clamp(uv.xz, state.cb.uv_min_max.xx, state.cb.uv_min_max.zz);
    }
    else if (PS_WMS == 3)
    {
      if (PS_FST == FALSE)
        uv.xz = FRACT(uv.xz);

      UINT4 uv_msk_fix = PS_UV_MSK_FIX(state.cb);
      uv.xz = FLOAT2((USHORT2(uv.xz * tex_size.xx) & USHORT2(uv_msk_fix.xx)) | USHORT2(uv_msk_fix.zz)) / tex_size.xx;
    }

    if (PS_REGION_RECT != FALSE && PS_WMT == 0)
    {
      uv.yw = FRACT(uv.yw);
    }
    else if (PS_REGION_RECT != FALSE && PS_WMT == 1)
    {
      uv.yw = SATURATE(uv.yw);
    }
    else if (PS_WMT == 2)
    {
      uv.yw = clamp(uv.yw, state.cb.uv_min_max.yy, state.cb.uv_min_max.ww);
    }
    else if (PS_WMT == 3)
    {
      if (PS_FST == FALSE)
        uv.yw = FRACT(uv.yw);

      UINT4 uv_msk_fix = PS_UV_MSK_FIX(state.cb);
      uv.yw = FLOAT2((USHORT2(uv.yw * tex_size.yy) & USHORT2(uv_msk_fix.yy)) | USHORT2(uv_msk_fix.ww)) / tex_size.yy;
    }
  }

  if (PS_REGION_RECT != FALSE)
  {
    // Normalized -> Integer Coordinates.
    uv = clamp(uv * state.cb.wh.zwzw + state.cb.st_range.xyxy, state.cb.st_range.xyxy, state.cb.st_range.zwzw);
  }

  return uv;
}

FLOAT4x4 sample_4c(IN_PARAM(PSMain, state), FLOAT4 uv)
{
  return FLOAT4x4(
    sample_c(state, uv.xy),
    sample_c(state, uv.zy),
    sample_c(state, uv.xw),
    sample_c(state, uv.zw)
  );
}

UINT4 sample_4_index(IN_PARAM(PSMain, state), FLOAT4 uv)
{
  FLOAT4 c;

  // Either GS will send a texture that contains a single alpha channel
  // Or we have an old RT (ie RGBA8) that contains index (4/8) in the alpha channel

  // Note: texture gather can't be used because of special clamping/wrapping
  // Also it doesn't support lod
  c.x = sample_c(state, uv.xy).a;
  c.y = sample_c(state, uv.zy).a;
  c.z = sample_c(state, uv.xw).a;
  c.w = sample_c(state, uv.zw).a;
  
  UINT4 i;
  
  if (PS_RTA_SRC_CORRECTION != FALSE)
  {
    i = UINT4(round(c * 128.25f)); // Denormalize value
  }
  else
  {
    i = UINT4(c * 255.5f); // Denormalize value
  }
  
  if (PS_PAL_FMT == 1)
    return i & 0xFu;
  if (PS_PAL_FMT == 2)
    return i >> 4;

  return i;
}

FLOAT4x4 sample_4p(IN_PARAM(PSMain, state), UINT4 u)
{
  return FLOAT4x4(
    sample_p(state, u.x),
    sample_p(state, u.y),
    sample_p(state, u.z),
    sample_p(state, u.w)
  );
}

uint fetch_raw_depth(IN_PARAM(PSMain, state))
{
  return uint(PS_READ_TEX_DEPTH(state, USHORT2(state.psinput.p.xy + state.cb.channel_shuffle_offset), 0) * EXP2_POS_32);
}

FLOAT4 fetch_raw_color(IN_PARAM(PSMain, state))
{
  if (PS_TEX_IS_FB != FALSE)
    return state.current_color;
  else
    return PS_READ_TEX(state, USHORT2(state.psinput.p.xy + state.cb.channel_shuffle_offset), 0);
}

FLOAT4 fetch_c(IN_PARAM(PSMain, state), USHORT2 uv)
{
  if (PS_TEX_IS_FB != FALSE)
    return state.current_color;
  else if (PS_TEX_IS_DEPTH != FALSE)
    return FLOAT4(PS_READ_TEX_DEPTH(state, uv, 0), 0.0f, 0.0f, 0.0f);
  else
    return PS_READ_TEX(state, uv, 0);
}

USHORT2 clamp_wrap_uv_depth(IN_PARAM(PSMain, state), USHORT2 uv)
{
  USHORT2 uv_out = uv;
  // Keep the full precision
  // It allow to multiply the ScalingFactor before the 1/16 coeff
  USHORT4 mask = USHORT4(PS_UV_MSK_FIX(state.cb) << 4);

  if (PS_WMS == PS_WMT)
  {
    if (PS_WMS == 2)
      uv_out = clamp(uv, mask.xy, mask.zw);
    else if (PS_WMS == 3)
      uv_out = (uv & mask.xy) | mask.zw;
  }
  else
  {
    if (PS_WMS == 2)
      uv_out.x = clamp(uv.x, mask.x, mask.z);
    else if (PS_WMS == 3)
      uv_out.x = (uv.x & mask.x) | mask.z;

    if (PS_WMT == 2)
      uv_out.y = clamp(uv.y, mask.y, mask.w);
    else if (PS_WMT == 3)
      uv_out.y = (uv.y & mask.y) | mask.w;
  }

  return uv_out;
}

FLOAT4 sample_depth(IN_PARAM(PSMain, state), FLOAT2 st)
{
  FLOAT2 uv_f = FLOAT2(clamp_wrap_uv_depth(state, USHORT2(st))) * float2_bcast(state.cb.scale_factor.x);

  if (PS_REGION_RECT != FALSE)
    uv_f = clamp(uv_f + state.cb.st_range.xy, state.cb.st_range.xy, state.cb.st_range.zw);

  USHORT2 uv = USHORT2(uv_f);
  FLOAT4 t = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

  if (PS_TALES_OF_ABYSS_HLE != FALSE)
  {
    // Warning: UV can't be used in channel effect
    USHORT depth = fetch_raw_depth(state);
    // Convert msb based on the palette
    t = PS_READ_PALETTE(state, USHORT2((depth >> 8) & 0xFFu, 0)) * 255.f;
  }
  else if (PS_URBAN_CHAOS_HLE != FALSE)
  {
    // Depth buffer is read as a RGB5A1 texture. The game try to extract the green channel.
    // So it will do a first channel trick to extract lsb, value is right-shifted.
    // Then a new channel trick to extract msb which will shifted to the left.
    // OpenGL uses a FLOAT32 format for the depth so it requires a couple of conversion.
    // To be faster both steps (msb&lsb) are done in a single pass.

    // Warning: UV can't be used in channel effect
    USHORT depth = fetch_raw_depth(state);

    // Convert lsb based on the palette
    t = PS_READ_PALETTE(state, USHORT2(depth & 0xFFu, 0)) * 255.f;

    // Msb is easier
    float green = float((depth >> 8) & 0xFFu) * 36.f;
    green = min(green, 255.0f);

    t.g += green;
  }
  else if (PS_DEPTH_FMT == 1)
  {
    t = convert_depth32_rgba8(fetch_c(state, uv).r);
  }
  else if (PS_DEPTH_FMT == 2)
  {
    t = convert_depth16_rgba8(fetch_c(state, uv).r);
  }
  else if (PS_DEPTH_FMT == 3)
  {
    t = fetch_c(state, uv) * 255.f;
  }

  // macOS 10.15 ICE's on bool3(t.rgb), so use != 0 instead
  if (PS_AEM_FMT == FMT_24)
    t.a = (PS_AEM == FALSE || any(VNOTEQUAL(t.rgb, FLOAT3(0, 0, 0)))) ? 255.f * state.cb.ta.x : 0.f;
  else if (PS_AEM_FMT == FMT_16)
    t.a = t.a >= 128.f ? 255.f * state.cb.ta.y : (PS_AEM == FALSE || any(VNOTEQUAL(t.rgb, FLOAT3(0, 0, 0)))) ? 255.f * state.cb.ta.x : 0.f;
  else if (PS_PAL_FMT != 0 && PS_TALES_OF_ABYSS_HLE == FALSE && PS_URBAN_CHAOS_HLE == FALSE)
    t = trunc(sample_4p(state, UINT4(t.aaaa))[0] * 255.0f + 0.05f);
  
  return t;
}

// MARK: Fetch a Single Channel

FLOAT4 fetch_red(IN_PARAM(PSMain, state))
{
  float rt = PS_TEX_IS_DEPTH != FALSE ? float(fetch_raw_depth(state) & 0xFFu) / 255.f : fetch_raw_color(state).r;
  return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_green(IN_PARAM(PSMain, state))
{
  float rt = PS_TEX_IS_DEPTH != FALSE ? float((fetch_raw_depth(state) >> 8) & 0xFFu) / 255.f : fetch_raw_color(state).g;
  return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_blue(IN_PARAM(PSMain, state))
{
  float rt = PS_TEX_IS_DEPTH != FALSE ? float((fetch_raw_depth(state) >> 16) & 0xFFu) / 255.f : fetch_raw_color(state).b;
  return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_alpha(IN_PARAM(PSMain, state))
{
  return sample_p_norm(state, fetch_raw_color(state).a) * 255.f;
}

FLOAT4 fetch_rgb(IN_PARAM(PSMain, state))
{
  FLOAT4 rt = fetch_raw_color(state);
  return FLOAT4(sample_p_norm(state, rt.r).r, sample_p_norm(state, rt.g).g, sample_p_norm(state, rt.b).b, 1) * 255.f;
}

FLOAT4 fetch_gXbY(IN_PARAM(PSMain, state))
{
  if (PS_TEX_IS_DEPTH != FALSE)
  {
    uint depth = fetch_raw_depth(state);
    uint bg = (depth >> (8 + state.cb.channel_shuffle_green_shift)) & 0xFFu;
    return float4_bcast(bg);
  }
  else
  {
    UINT4 rt = UINT4(fetch_raw_color(state) * 255.5f);
    uint green = (rt.g >> state.cb.channel_shuffle_green_shift) & state.cb.channel_shuffle_green_mask;
    uint blue  = (rt.b >> state.cb.channel_shuffle_blue_shift)  & state.cb.channel_shuffle_blue_mask;
    return float4_bcast(green | blue);
  }
}

FLOAT4 sample_color(IN_PARAM(PSMain, state), FLOAT2 st)
{
  if (PS_TCOFFSETHACK != FALSE)
    st += state.cb.tc_offset;

  FLOAT4 t;
  FLOAT4x4 c;
  FLOAT2 dd;

  if (PS_LTF == FALSE && PS_AEM_FMT == FMT_32 && PS_PAL_FMT == 0 && PS_REGION_RECT == FALSE && PS_WMS < 2 && PS_WMT < 2)
  {
    c[0] = sample_c(state, st);
  }
  else
  {
    FLOAT4 uv;
    if (PS_LTF != FALSE)
    {
      uv = st.xyxy + state.cb.half_texel;
      dd = FRACT(uv.xy * state.cb.wh.zw);
      if (PS_FST == FALSE)
      {
        // Background in Shin Megami Tensei Lucifers
        // I suspect that uv isn't a standard number, so fract is outside of the [0;1] range
        dd = SATURATE(dd);
      }
    }
    else
    {
      uv = st.xyxy;
    }

    uv = clamp_wrap_uv(state, uv);

    if (PS_PAL_FMT != 0)
      c = sample_4p(state, sample_4_index(state, uv));
    else
      c = sample_4c(state, uv);
  }

  for (int i = 0; i < 4; i++)
  {
    // macOS 10.15 ICE's on bool3(c[i].rgb), so use != 0 instead
    if (PS_AEM_FMT == FMT_24)
      c[i].a = PS_AEM == FALSE || any(VNOTEQUAL(c[i].rgb, FLOAT3(0, 0, 0))) ? state.cb.ta.x : 0.f;
    else if (PS_AEM_FMT == FMT_16)
      c[i].a = c[i].a >= 0.5 ? state.cb.ta.y : PS_AEM == FALSE || any(VNOTEQUAL(INT3(c[i].rgb * 255.0f) & 0xF8, INT3(0, 0, 0))) ? state.cb.ta.x : 0.f;
  }

  if (PS_LTF != FALSE)
    t = MIX(MIX(c[0], c[1], dd.x), MIX(c[2], c[3], dd.x), dd.y);
  else
    t = c[0];

  if (PS_AEM_FMT == FMT_32 && PS_PAL_FMT == 0 && PS_RTA_SRC_CORRECTION != FALSE)
    t.a = t.a * (128.5f / 255.0f);
    
  // The 0.05f helps to fix the overbloom of sotc
  // I think the issue is related to the rounding of texture coodinate. The linear (from fixed unit)
  // interpolation could be slightly below the correct one.
  
  return trunc(t * 255.f + 0.05f);
}

FLOAT4 tfx(IN_PARAM(PSMain, state), FLOAT4 T, FLOAT4 C)
{
  FLOAT4 C_out;
  FLOAT4 FxT = trunc((C * T) / 128.f);
  if (PS_TFX == 0)
    C_out = FxT;
  else if (PS_TFX == 1)
    C_out = T;
  else if (PS_TFX == 2)
    C_out = FLOAT4(FxT.rgb, T.a) + C.a;
  else if (PS_TFX == 3)
    C_out = FLOAT4(FxT.rgb + C.a, T.a);
  else
    C_out = C;

  if (PS_TCC == FALSE)
    C_out.a = C.a;

  // Clamp only when it is useful
  if (PS_TFX == 0 || PS_TFX == 2 || PS_TFX == 3)
    C_out = min(C_out, 255.f);

  return C_out;
}

bool atst(IN_PARAM(PSMain, state), FLOAT4 C)
{
  float a = C.a;
  switch (PS_ATST)
  {
    case PS_ATST_NONE:
      break; // Nothing to do
    case PS_ATST_LEQUAL:
      if (a > state.cb.aref)
        return false;
      break;
    case PS_ATST_GEQUAL:
      if (a < state.cb.aref)
        return false;
      break;
    case PS_ATST_EQUAL:
      if (abs(a - state.cb.aref) > 0.5f)
        return false;
      break;
    case PS_ATST_NOTEQUAL:
      if (abs(a - state.cb.aref) < 0.5f)
        return false;
      break;
  }
  return true;
}

void fog(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, C), float f)
{
  if (PS_FOG != FALSE)
    C.rgb = trunc(MIX(state.cb.fog_color, C.rgb, (f * 255.0f) / 256.0f));
}

FLOAT4 ps_color(IN_PARAM(PSMain, state))
{
  FLOAT2 st, st_int;
  if (PS_FST == FALSE)
  {
    st = state.psinput.t.xy / state.psinput.t.w;
    st_int = state.psinput.ti.zw / state.psinput.t.w;
  }
  else
  {
    // Note: xy are normalized coordinates
    st = state.psinput.ti.xy;
    st_int = state.psinput.ti.zw;
  }

  FLOAT4 T;
  if (PS_CHANNEL == 1)
    T = fetch_red(state);
  else if (PS_CHANNEL == 2)
    T = fetch_green(state);
  else if (PS_CHANNEL == 3)
    T = fetch_blue(state);
  else if (PS_CHANNEL == 4)
    T = fetch_alpha(state);
  else if (PS_CHANNEL == 5)
    T = fetch_rgb(state);
  else if (PS_CHANNEL == 6)
    T = fetch_gXbY(state);
  else if (PS_DEPTH_FMT != 0)
    T = sample_depth(state, st_int);
  else
    T = sample_color(state, st);

  if (PS_SHUFFLE != FALSE && PS_SHUFFLE_SAME == FALSE && PS_READ16_SRC == FALSE &&
    !(PS_PROCESS_BA == SHUFFLE_READWRITE && PS_PROCESS_RG == SHUFFLE_READWRITE))
  {
    UINT4 denorm_c_before = UINT4(T);
    if ((PS_PROCESS_BA & SHUFFLE_READ) != FALSE)
    {
      T.r = float((denorm_c_before.b << 3) & 0xF8u);
      T.g = float(((denorm_c_before.b >> 2) & 0x38u) | ((denorm_c_before.a << 6) & 0xC0u));
      T.b = float((denorm_c_before.a << 1) & 0xF8u);
      T.a = float(denorm_c_before.a & 0x80u);
    }
    else
    {
      T.r = float((denorm_c_before.r << 3) & 0xF8u);
      T.g = float(((denorm_c_before.r >> 2) & 0x38u) | ((denorm_c_before.g << 6) & 0xC0u));
      T.b = float((denorm_c_before.g << 1) & 0xF8u);
      T.a = float(denorm_c_before.g & 0x80u);
    }
    
    T.a = (T.a >= 127.5 ? state.cb.ta.y : PS_AEM == FALSE || any(VNOTEQUAL((INT3(T.rgb) & 0xF8), INT3(0, 0, 0))) ? state.cb.ta.x : 0.f) * 255.f;
  }

  FLOAT4 C = tfx(state, T, PS_IIP != FALSE ? state.psinput.c : state.psinput.fc);

  fog(state, C, state.psinput.t.z);

  return C;
}

void ps_fbmask(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, C))
{
  if (PS_FBMASK != FALSE)
  {
    float multi = (PS_COLCLIP_HW != FALSE) ? 65535.0 : 255.5;
    C = FLOAT4((UINT4(INT4(C)) & (state.cb.fbmask ^ 0xffu)) | (UINT4(state.current_color * FLOAT4(multi, multi, multi, 255)) & state.cb.fbmask));
  }
}

void ps_dither(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, C), float As)
{
  if (PS_DITHER == 0 || PS_DITHER == 3)
    return;
  USHORT2 fpos;
  if (PS_DITHER == 2)
    fpos = USHORT2(state.psinput.p.xy);
  else
    fpos = USHORT2(state.psinput.p.xy * float2_bcast(state.cb.scale_factor.y));
  float value = state.cb.dither_matrix[fpos.y & 3u][fpos.x & 3u];

  // The idea here is we add on the dither amount adjusted by the alpha before it goes to the hw blend
  // so after the alpha blend the resulting value should be the same as (Cs - Cd) * As + Cd + Dither.
  if (PS_DITHER_ADJUST != FALSE)
  {
    float Alpha = PS_BLEND_C == 2 ? state.cb.alpha_fix : As;
    value *= Alpha > 0.f ? min(1.f / Alpha, 1.f) : 1.f;
  }

  if (PS_ROUND_INV != FALSE)
    C.rgb -= value;
  else
    C.rgb += value;
}

void ps_color_clamp_wrap(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, C))
{
  // When dithering the bottom 3 bits become meaningless and cause lines in the picture
  // so we need to limit the color depth on dithered items
  if (SW_BLEND || (PS_DITHER > 0 && PS_DITHER < 3) || PS_FBMASK != FALSE)
  {
    if (PS_DST_FMT == FMT_16 && PS_BLEND_MIX == 0 && PS_ROUND_INV != FALSE)
      C.rgb += 7.f; // Need to round up, not down since the shader will invert

    // Correct the Color value based on the output format
    if (PS_COLCLIP == 0 && PS_COLCLIP_HW == 0)
      C.rgb = clamp(C.rgb, 0.f, 255.f); // Standard Clamp

    // FIXME rouding of negative float?
    // compiler uses trunc but it might need floor

    // Warning: normally blending equation is mult(A, B) = A * B >> 7. GPU have the full accuracy
    // GS: Color = 1, Alpha = 255 => output 1
    // GPU: Color = 1/255, Alpha = 255/255 * 255/128 => output 1.9921875
    // In 16 bits format, only 5 bits of colors are used. It impacts shadows computation of Castlevania
    if (PS_DST_FMT == FMT_16 && PS_DITHER != 3 && (PS_BLEND_MIX == 0 || PS_DITHER != FALSE))
      C.rgb = FLOAT3(SHORT3(C.rgb) & 0xF8);
    else if (PS_COLCLIP == 1 || PS_COLCLIP_HW == 1)
      C.rgb = FLOAT3(SHORT3(C.rgb) & 0xFF);
  }
  else if (PS_DST_FMT == FMT_16 && PS_DITHER != 3 && PS_BLEND_MIX == 0 && PS_BLEND_HW == 0)
    C.rgb = FLOAT3(SHORT3(C.rgb) & 0xF8);
}

#define PICK3(SELECTOR, ZERO, ONE, TWO) ((SELECTOR) == 0 ? (ZERO) : (SELECTOR) == 1 ? (ONE) : (TWO))

void ps_blend(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, Color), IN_OUT_PARAM(FLOAT4, As_rgba))
{
  float As = As_rgba.a;
  
  if (SW_BLEND)
  {
    // PABE
    if (PS_PABE != FALSE)
    {
      // As_rgba needed for accumulation blend to manipulate Cd.
      // No blending so early exit
      if (As < 1.f)
      {
        As_rgba.rgb = float3_bcast(0.f);
        return;
      }

      As_rgba.rgb = float3_bcast(1.f);
    }

    float Ad = PS_RTA_CORRECTION != FALSE ?
      trunc(state.current_color.a * 128.1f) / 128.f : trunc(state.current_color.a * 255.1f) / 128.f;

    if (PS_SHUFFLE != FALSE && NEEDS_RT)
    {
      UINT4 denorm_rt = UINT4(state.current_color);
      if ((PS_PROCESS_BA & SHUFFLE_WRITE) != 0)
      {
        state.current_color.r = float((denorm_rt.b << 3) & 0xF8u);
        state.current_color.g = float(((denorm_rt.b >> 2) & 0x38u) | ((denorm_rt.a << 6) & 0xC0u));
        state.current_color.b = float((denorm_rt.a << 1) & 0xF8u);
        state.current_color.a = float(denorm_rt.a & 0x80u);
      }
      else
      {
        state.current_color.r = float((denorm_rt.r << 3) & 0xF8u);
        state.current_color.g = float(((denorm_rt.r >> 2) & 0x38u) | ((denorm_rt.g << 6) & 0xC0u));
        state.current_color.b = float((denorm_rt.g << 1) & 0xF8u);
        state.current_color.a = float(denorm_rt.g & 0x80u);
      }
    }
    float multi = PS_COLCLIP_HW != FALSE ? 65535.0 : 255.5;
    FLOAT3 Cd = trunc(state.current_color.rgb * multi);
    FLOAT3 Cs = Color.rgb;

    FLOAT3 A = PICK3(PS_BLEND_A, Cs, Cd, FLOAT3(0.f, 0.f, 0.f));
    FLOAT3 B = PICK3(PS_BLEND_B, Cs, Cd, FLOAT3(0.f, 0.f, 0.f));
    float  C = PICK3(PS_BLEND_C, As, Ad, state.cb.alpha_fix);
    FLOAT3 D = PICK3(PS_BLEND_D, Cs, Cd, FLOAT3(0.f, 0.f, 0.f));

    // As/Af clamp alpha for Blend mix
    // We shouldn't clamp blend mix with blend hw 1 as we want alpha higher
    float C_clamped = C;
    if (PS_BLEND_MIX > 0 && PS_BLEND_HW != 1 && PS_BLEND_HW != 2)
      C_clamped = SATURATE(C_clamped);

    if (PS_BLEND_A == PS_BLEND_B)
      Color.rgb = D;
    // In blend_mix, HW adds on some alpha factor * dst.
    // Truncating here wouldn't quite get the right result because it prevents the <1 bit here from combining with a <1 bit in dst to form a ≥1 amount that pushes over the truncation.
    // Instead, apply an offset to convert HW's round to a floor.
    // Since alpha is in 1/128 increments, subtracting (0.5 - 0.5/128 == 127/256) would get us what we want if GPUs blended in full precision.
    // But they don't.  Details here: https://github.com/PCSX2/pcsx2/pull/6809#issuecomment-1211473399
    // Based on the scripts at the above link, the ideal choice for Intel GPUs is 126/256, AMD 120/256.  Nvidia is a lost cause.
    // 124/256 seems like a reasonable compromise, providing the correct answer 99.3% of the time on Intel (vs 99.6% for 126/256), and 97% of the time on AMD (vs 97.4% for 120/256).
    else if (PS_BLEND_MIX == 2)
      Color.rgb = ((A - B) * C_clamped + D) + (124.f/256.f);
    else if (PS_BLEND_MIX == 1)
      Color.rgb = ((A - B) * C_clamped + D) - (124.f/256.f);
    else
      Color.rgb = trunc((A - B) * C + D);

    if (PS_BLEND_HW == 1)
    {
      // As or Af
      As_rgba.rgb = float3_bcast(C);
      // Subtract 1 for alpha to compensate for the changed equation,
      // if c.rgb > 255.0f then we further need to adjust alpha accordingly,
      // we pick the lowest overflow from all colors because it's the safest,
      // we divide by 255 the color because we don't know Cd value,
      // changed alpha should only be done for hw blend.
      FLOAT3 alpha_compensate = max(float3_bcast(1.f), Color.rgb / float3_bcast(0.f));
      As_rgba.rgb -= alpha_compensate;
    }
    else if (PS_BLEND_HW == 2)
    {
      // Since we can't do Cd*(Alpha + 1) - Cs*Alpha in hw blend
      // what we can do is adjust the Cs value that will be
      // subtracted, this way we can get a better result in hw blend.
      // Result is still wrong but less wrong than before.
      float division_alpha = 1.f + C;
      Color.rgb /= float3_bcast(division_alpha);
    }
    else if (PS_BLEND_HW == 3)
    {
      // As, Ad or Af clamped.
      As_rgba.rgb = float3_bcast(C_clamped);
      // Cs*(Alpha + 1) might overflow, if it does then adjust alpha value
      // that is sent on second output to compensate.
      FLOAT3 overflow_check = (Color.rgb - float3_bcast(0.f)) / 255.f;
      FLOAT3 alpha_compensate = max(float3_bcast(0.f), overflow_check);
      As_rgba.rgb -= alpha_compensate;
    }
  }
  else
  {
    if (PS_BLEND_HW == 1)
    {
      // Needed for Cd * (As/Ad/F + 1) blending modes
      Color.rgb = float3_bcast(255.f);
    }
    else if (PS_BLEND_HW == 2)
    {
      // Cd*As,Cd*Ad or Cd*F
      float Alpha = PS_BLEND_C == 2 ? state.cb.alpha_fix : As;
      Color.rgb = float3_bcast(SATURATE(Alpha - 1.f) * 255.f);
    }
    else if (PS_BLEND_HW == 3 && PS_RTA_CORRECTION == 0)
    {
      // Needed for Cs*Ad, Cs*Ad + Cd, Cd - Cs*Ad
      // Multiply Color.rgb by (255/128) to compensate for wrong Ad/255 value when rgb are below 128.
      // When any color channel is higher than 128 then adjust the compensation automatically
      // to give us more accurate colors, otherwise they will be wrong.
      // The higher the value (>128) the lower the compensation will be.
      float max_color = max(max(Color.r, Color.g), Color.b);
      float color_compensate = 255.f / max(128.f, max_color);
      Color.rgb *= float3_bcast(color_compensate);
    }
  }
}

void ps_shuffle(IN_PARAM(PSMain, state), IN_OUT_PARAM(FLOAT4, C))
{
  if (PS_SHUFFLE != FALSE)
  {
    if ((PS_SHUFFLE_SAME == FALSE) && (PS_READ16_SRC == FALSE) &&
      !(PS_PROCESS_BA == SHUFFLE_READWRITE && PS_PROCESS_RG == SHUFFLE_READWRITE))
    {
      UINT4 denorm_c_after = UINT4(C);
      if ((PS_PROCESS_BA & SHUFFLE_READ) != 0)
      {
        C.b = float(((denorm_c_after.r >> 3) & 0x1Fu) | ((denorm_c_after.g << 2) & 0xE0u));
        C.a = float(((denorm_c_after.g >> 6) & 0x3u) | ((denorm_c_after.b >> 1) & 0x7Cu) | (denorm_c_after.a & 0x80u));
      }
      else
      {
        C.r = float(((denorm_c_after.r >> 3) & 0x1Fu) | ((denorm_c_after.g << 2) & 0xE0u));
        C.g = float(((denorm_c_after.g >> 6) & 0x3u) | ((denorm_c_after.b >> 1) & 0x7Cu) | (denorm_c_after.a & 0x80u));
      }
    }

    // Special case for 32bit input and 16bit output, shuffle used by The Godfather
    if (PS_SHUFFLE_SAME != FALSE)
    {
      UINT4 denorm_c = UINT4(C);
      
      if ((PS_PROCESS_BA & SHUFFLE_READ) != 0u)
        C = float4_bcast(float((denorm_c.b & 0x7Fu) | (denorm_c.a & 0x80u)));
      else
        C.ga = C.rg;
    }
    // Copy of a 16bit source in to this target
    else if (PS_READ16_SRC != FALSE)
    {
      UINT4 denorm_c = UINT4(C);
      UINT2 denorm_TA = UINT2(state.cb.ta * 255.5f);
      
      C.rb = float2_bcast(float((denorm_c.r >> 3) | (((denorm_c.g >> 3) & 0x7u) << 5)));
      C.ga = float2_bcast(float((denorm_c.g >> 6) | ((denorm_c.b >> 3) << 2) | (denorm_TA.x & 0x80u)));
    }
    else if (PS_SHUFFLE_ACROSS != FALSE)
    {
      if (PS_PROCESS_BA == SHUFFLE_READWRITE && PS_PROCESS_RG == SHUFFLE_READWRITE)
      {
        C.br = C.rb;
        C.ag = C.ga;
      }
      else if ((PS_PROCESS_BA & SHUFFLE_READ) != 0u)
      {
        C.rb = C.bb;
        C.ga = C.aa;
      }
      else
      {
        C.rb = C.rr;
        C.ga = C.gg;
      }
    }
  }
}

PSOutputGeneric ps_main_impl(IN_OUT_PARAM(PSMain, state))
{
  PSOutputGeneric psout;
  psout.c0 = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
  psout.c1 = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
  psout.depth = 0.0f;

  float input_z = state.psinput.p.z;
  if (PS_ZFLOOR != FALSE)
    input_z = floor(input_z * EXP2_POS_32) * EXP2_MIN_32;

  if (PS_ZTST == ZTST_GEQUAL || PS_ZTST == ZTST_GREATER)
  {
    if (PS_ZTST == ZTST_GEQUAL && input_z < state.current_depth)
      discard_all(state);
    if (PS_ZTST == ZTST_GREATER && input_z <= state.current_depth)
      discard_all(state);
  }

  if ((uint(PS_SCANMSK) & 2u) != 0u)
  {
    if ((uint(state.psinput.p.y) & 1u) == (uint(PS_SCANMSK) & 1u))
      discard_all(state);
  }

  if (PS_DATE >= 5)
  {
    // 1 => DATM == 0, 2 => DATM == 1
    float rt_a = PS_WRITE_RG != FALSE ? state.current_color.g : state.current_color.a;
    bool bad = PS_RTA_CORRECTION != FALSE ?
      ((uint(PS_DATE) & 3u) == 1u ? (rt_a > (254.5f / 255.f)) : (rt_a < (254.5f / 255.f))) :
      ((uint(PS_DATE) & 3u) == 1u ? (rt_a > 0.5) : (rt_a < 0.5));

    if (bad)
      discard_all(state);
  }

  if (PS_DATE == 3)
  {
    float stencil_ceil = PS_READ_PRIMID(state, UINT2(state.psinput.p.xy));
    // Note prim_id == stencil_ceil will be the primitive that will update
    // the bad alpha value so we must keep it.
    if (float(state.prim_id) > stencil_ceil)
      discard_all(state);
  }

  FLOAT4 C = ps_color(state);

  // Must be done before alpha correction

  if (PS_AA1 != PS_AA1_NONE)
  {
    float cov = PS_AA1 == PS_AA1_LINE
      ? SATURATE(state.cb.line_cov_scale * (1.f - abs(state.psinput.inv_cov))) // Blur only outer part of the line by scaling coverage.
      : SATURATE(1.f - abs(state.psinput.inv_cov));
    if (PS_ABE == FALSE || floor(C.a) == 128.f) // The coverage is only used if the fragment alpha is 128.
      C.a = 128.f * cov;
  }
  else if (PS_FIXED_ONE_A != FALSE)
  {
    // AA (Fixed one) will output a coverage of 1.0 as alpha
    C.a = 128.0f;
  }

  bool atst_pass = atst(state, C);
  if (PS_AFAIL == AFAIL_KEEP && !atst_pass)
    discard_all(state);

  FLOAT4 alpha_blend = float4_bcast(0.f);
  if (SW_AD_TO_HW)
  {
    alpha_blend = float4_bcast(PS_RTA_CORRECTION != FALSE ?
      trunc(state.current_color.a * 128.f) / 128.f : trunc(state.current_color.a * 255.5f) / 128.f);
  }
  else
  {
    alpha_blend = float4_bcast(C.a / 128.f);
  }

  if (PS_DST_FMT == FMT_16)
  {
    float A_one = 128.f;
    C.a = (PS_FBA != FALSE) ? A_one : step(128.f, C.a) * A_one;
  }
  else if (PS_DST_FMT == FMT_32 && PS_FBA != FALSE)
  {
    if (C.a < 128.f)
      C.a += 128.f;
  }

  // Get first primitive that will write a failing alpha value
  if (PS_DATE == 1)
  {
    // DATM == 0, Pixel with alpha equal to 1 will failed (128-255)
    psout.c0 = float4_bcast(C.a > 127.5f ? float(state.prim_id) : float(PRIMID_MAX));
    return psout;
  }
  else if (PS_DATE == 2)
  {
    // DATM == 1, Pixel with alpha equal to 0 will failed (0-127)
    psout.c0 = float4_bcast(C.a < 127.5f ? float(state.prim_id) : PRIMID_MAX);
    return psout;
  }

  ps_blend(state, C, alpha_blend);

  ps_shuffle(state, C);

  ps_dither(state, C, alpha_blend.a);

  // Color clamp/wrap needs to be done after sw blending and dithering
  ps_color_clamp_wrap(state, C);

  ps_fbmask(state, C);

  // Use alpha blend factor to determine whether to update A.
  if (PS_AFAIL == AFAIL_RGB_ONLY_DSB)
    alpha_blend.a = float(atst_pass);

  if (PS_NO_COLOR == FALSE)
  {
    psout.c0.a = PS_RTA_CORRECTION != FALSE ? C.a / 128.f : C.a / 255.f;
    psout.c0.rgb = PS_COLCLIP_HW != FALSE ? FLOAT3(C.rgb / 65535.f) : C.rgb / 255.f;
  }
  if (PS_NO_COLOR1 == FALSE)
    psout.c1 = alpha_blend;

  if (PS_ZCLAMP != FALSE)
    input_z = min(input_z, state.cb.max_depth);

  if (PS_AA1 == PS_AA1_TRIANGLE_SW_Z && state.psinput.interior == 0)
    discard_depth(state, input_z); // No depth update for triangle edges.

  if (!atst_pass)
  {
    if (PS_AFAIL == AFAIL_RGB_ONLY_SW_Z || PS_AFAIL == AFAIL_RGB_ONLY)
      psout.c0.a = state.current_color.a; // discard alpha
    else if (PS_AFAIL == AFAIL_ZB_ONLY)
      discard_color(state, psout.c0);

    if (PS_AFAIL == AFAIL_RGB_ONLY_SW_Z || PS_AFAIL == AFAIL_FB_ONLY)
      discard_depth(state, input_z);
  }

  psout.depth = input_z;

  return psout;
}


void main()
{
  PSMain state;
  state.psinput = GetPSInput();
  state.cb = GetPSUniforms();
  state.tex = 0; // unused
  state.tex_depth = 0; // unused
  state.palette = 0; // unused
  state.prim_id_tex = 0; // unused
	state.tex_sampler = 0; // unused
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

	#if PS_RETURN_COLOR
		o_col0 = psout.c0;
		#if !PS_NO_COLOR1
			o_col1 = psout.c1;
		#endif
	#elif PS_RETURN_COLOR_ROV
		psout.c0 = mix(psout.c0, state.current_color, equal(FbMask, uvec4(0xFFu))); // channel masking
		if (!state.color_discarded)
			imageStore(RtImageRov, coord, psout.c0);
	#endif
	
	// Writing back depth
	#if PS_RETURN_DEPTH
		gl_FragDepth = psout.depth;
		#if PCSX2_OPENGL && PS_FEEDBACK_LOOP_IS_NEEDED_DEPTH && PS_NO_COLOR1 && (DEPTH_FEEDBACK_SUPPORT == 2)
			// Depth as color write. For depth as color feedback we write to both
			// color copy and real depth to avoid having to copy back to real depth.
			// Warning: do not write o_col1 until the end since the value might
			// be needed for FB fetch in sample_from_depth().
			o_col1 = psout.depth;
		#endif
	#elif PS_RETURN_DEPTH_ROV
		if (!state.depth_discarded)
			imageStore(DepthImageRov, coord, vec4(psout.depth, 0, 0, 1.0f));
	#endif

	#if PS_ROV_COLOR || PS_ROV_DEPTH
		endInvocationInterlockARB();
	#endif
}

#endif

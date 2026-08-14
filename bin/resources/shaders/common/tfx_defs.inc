#ifdef __METAL_VERSION__
  #define FALSE false
#else
  #define FALSE 0

	// Constants for non-Metal backend. In Metal, these are imported from C++ headers.
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
#endif

/// Vertex shader constant buffer fields. Shared by all shaders except Metal.
#define VS_UNIFORMS(X) \
	X(FLOAT2, vertex_scale) \
	X(FLOAT2, vertex_offset) \
	X(FLOAT2, texture_scale) \
	X(FLOAT2, texture_offset) \
	X(FLOAT2, point_size) \
	X(uint, max_depth_vs) \
	X(float, line_aa1_width)

/// Generic VS constant buffer for passing to shared shader code.
struct VSUniformsGeneric
{
	#define X(TYPE, NAME) TYPE NAME;
		VS_UNIFORMS(X)
	#undef X
};

/// Vertex shader push constant fields for non-Metal backends.
#define VS_PUSH_CONSTANTS(X) \
	X(uint, BaseVertex) \
	X(uint, BaseIndex) \
	X(uint, pad_cb2_0) \
	X(uint, pad_cb2_1)

/// Generic VS inputs for passing to shared shader code.
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

/// Generic VS outputs for use shared shader code to pass values.
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

/// Pixel shader constant buffer fields.
/// Shared by all shaders except Metal, which defines its own version.
#define PS_UNIFORMS(X) \
  X(FLOAT3, fog_color) \
  X(float, aref) \
	X(FLOAT4, wh)  \
	X(FLOAT2, ta) \
	X(float, max_depth_ps) \
	X(float, alpha_fix) \
	X(UINT4, fbmask) \
	X(FLOAT4, half_texel) \
  X(FLOAT4, uv_min_max) \
	X(FLOAT4, lod_params) \
	X(FLOAT4, st_range) \
  X(uint, channel_shuffle_blue_mask) \
  X(uint, channel_shuffle_blue_shift) \
  X(uint, channel_shuffle_green_mask) \
  X(uint, channel_shuffle_green_shift) \
	X(FLOAT2, channel_shuffle_offset) \
	X(FLOAT2, tc_offset) \
	X(FLOAT2, st_scale) \
	X(FLOAT4x4, dither_matrix) \
	X(FLOAT4, scale_factor) \
	X(float, line_cov_scale) \
	X(float, _pad0) \
	X(float, _pad1) \
	X(float, _pad2)

/// Generic PS constant buffer for passing to shared shader code.
struct PSUniformsGeneric
{
  #define X(TYPE, NAME) TYPE NAME;
		PS_UNIFORMS(X)
	#undef X
};

/// Generic PS inputs for passing to shared shader code.
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

/// Main state of the pixel shader invocation for shared code.
/// Metal defines it's own version with identically named fields.
#ifndef __METAL_VERSION__
struct PSMainState
{
	// Fields used by Metal for texture slots. Unused by other backends.
	uint tex;
	uint tex_depth;
	uint palette;
	uint prim_id_tex;
	uint tex_sampler;

	// Fields used by all backends.
	FLOAT4 current_color;
	float current_depth;
	uint prim_id;
	bool color_discarded;
	bool depth_discarded;
	PSInputGeneric psin;
	PSUniformsGeneric cb;
};
#endif

/// Generic PS outputs for shared shader code to pass values.
struct PSOutputGeneric
{
	FLOAT4 c0;
	FLOAT4 c1;
	float depth;
};

// Metal defines it's own version of these conversion functions.
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

STATIC INT3 int3_bcast(int val)
{
	return INT3(val, val, val);
}

STATIC bool any_nonzero(FLOAT3 val)
{
	return any(VNOTEQUAL(val, float3_bcast(0.0f)));
}

STATIC bool any_nonzero(INT3 val)
{
	return any(VNOTEQUAL(val, int3_bcast(0)));
}
#ifdef __METAL_VERSION__
  #define FALSE false
#else
  #define FALSE 0
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

/// Generic VS output for use by shared shader code.
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

/// Pixel shader constant buffer fields. Shared by all shaders except Metal.
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

// Main state of the pixel shader invocation.
#ifdef __METAL_VERSION__
  struct PSMainState
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
    const thread MainPSIn& psin;
    constant GSMTLMainPSUniform& cb;

    PSMainState(const thread MainPSIn& psin, constant GSMTLMainPSUniform& cb): psin(psin), cb(cb) {}
  };
#else
  struct PSMainState
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
    PSInputGeneric psin;
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
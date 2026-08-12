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
FLOAT4 convert_depth32_rgba8(float value)
{
	uint val = FLOAT_BITCAST_UINT(value * EXP2_POS_32);
	return UINT_BITCAST_FLOAT4(val);
}

FLOAT4 convert_depth16_rgba8(float value)
{
	uint val = FLOAT_BITCAST_UINT(value * EXP2_POS_32);
	return FLOAT4(UINT4(val << 3, val >> 2, val >> 7, val >> 8) & UINT4(0xf8, 0xf8, 0xf8, 0x80));
}
#endif
#define STATIC static
#define DFDX dfdx

#define VS_INPUT_TYPE MainVSIn
#define VS_OUTPUT_TYPE MainVSOut

#define VERTICES_PARAM(NAME) device const GSMTLMainVertex* NAME [[buffer(GSMTLBufferIndexHWVertices)]]
#define INDICES_PARAM(NAME) device const ushort* NAME [[buffer(GSMTLBufferIndexHWIndices), function_constant(VS_NEEDS_INDEX_BUFFER)]]
#define IN_PARAM(TYPE, NAME) thread const TYPE & NAME
#define OUT_PARAM(TYPE, NAME) thread TYPE & NAME
#define IN_OUT_VAR(TYPE, NAME) thread TYPE & NAME

#define FLOAT2 float2
#define FLOAT3 float3
#define FLOAT4 float4
#define FLOAT2x2 float2x2
#define FLOAT2x4 float2x4
#define UINT2 uint2
#define UINT3 uint3
#define UINT4 uint4
#define USHORT2 ushort2
#define USHORT3 ushort3
#define USHORT4 ushort4
#define INT2 int2
#define INT3 int3
#define INT4 int4

#define PRIMID_MAX FLT_MAX

#define VS_Y_FLIP -1.0f

#define PS_UV_MSK_FIX(CB) as_type<uint4>(CB.uv_min_max)
#define PS_DISCARD discard_fragment()
#define PS_SAMPLE_TEX(STATE, ...) STATE.tex.sample(STATE.sampler, __VA_ARGS__)
#define PS_SAMPLE_TEX__LOD(STATE, ...) STATE.tex.sample(STATE.sampler, __VA_ARGS__)
#define PS_SAMPLE_TEX_DEPTH(STATE, ...) STATE.tex_depth.sample(STATE.sampler, __VA_ARGS__)
#define PS_SAMPLE_TEX_DEPTH_LOD(STATE, ...) STATE.tex_depth.sample(STATE.sampler, __VA_ARGS__)
#define PS_READ_TEX(STATE, POS, LOD) STATE.tex.read((POS), (LOD)) 
#define PS_READ_TEX_DEPTH(STATE, POS, LOD) STATE.tex_depth.read((POS), (LOD)) 
#define PS_READ_PALETTE(STATE, IDX) STATE.palette.read(UINT2((IDX), 0))
#define PS_GET_TEX_DIMS(STATE) return UINT2(STATE.tex.get_width(), STATE.tex.get_height())
#define PS_GET_TEX_DEPTH_DIMS(STATE) return UINT2(STATE.tex_depth.get_width(), STATE.tex_depth.get_height())
#define PS_CURRENT_COLOR current_color

#ifndef VS_EXPAND_NONE
#define VS_EXPAND_NONE VSExpand::None
#define VS_EXPAND_POINT VSExpand::Point
#define VS_EXPAND_LINE VSExpand::Line
#define VS_EXPAND_SPRITE VSExpand::Sprite
#define VS_EXPAND_LINE_AA1 VSExpand::LineAA1
#define VS_EXPAND_TRIANGLE_AA1 VSExpand::TriangleAA1
#endif

#ifndef ZTST_GEQUAL
#define ZTST_GEQUAL ZTST::GEQUAL
#define ZTST_GREATER ZTST::GREATER
#endif

#ifndef AFAIL_KEEP
#define AFAIL_KEEP AFAIL::KEEP
#define AFAIL_FB_ONLY AFAIL::FB_ONLY
#define AFAIL_ZB_ONLY AFAIL::ZB_ONLY
#define AFAIL_RGB_ONLY AFAIL::RGB_ONLY
#define AFAIL_RGB_ONLY_DSB AFAIL::RGB_ONLY_DSB
#define AFAIL_RGB_ONLY_SW_Z AFAIL::RGB_ONLY_SW_Z
#endif

#ifndef PS_ATST_NONE
#define PS_ATST_NONE ATST::NONE
#define PS_ATST_LEQUAL ATST::LEQUAL
#define PS_ATST_GEQUAL ATST::GEQUAL
#define PS_ATST_EQUAL ATST::EQUAL
#define PS_ATST_NOTEQUAL ATST::NOTEQUAL
#endif

#ifndef PS_AA1_NONE
#define PS_AA1_NONE AA1::NONE
#define PS_AA1_LINE AA1::LINE
#define PS_AA1_TRIANGLE AA1::TRIANGLE
#define PS_AA1_TRIANGLE_SW_Z AA1::TRIANGLE_SW_Z
#endif

#ifndef PS_ROV_DEPTH_NONE
#define PS_ROV_DEPTH_NONE ROV_DEPTH::NONE
#define PS_ROV_DEPTH_READ_WRITE ROV_DEPTH::READ_WRITE
#define PS_ROV_DEPTH_READ_ONLY ROV_DEPTH::READ_ONLY
#endif

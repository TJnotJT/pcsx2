#define STATIC
#define DFDX dFdx
#define DFDY dFdy

#define VERTICES_PARAM(NAME) uint vertices_ignored
#define INDICES_PARAM(NAME) uint indices_ignored
#define IN_PARAM(TYPE, NAME) TYPE NAME
#define OUT_PARAM(TYPE, NAME) out TYPE NAME
#define IN_OUT_VAR(TYPE, NAME) inout TYPE NAME

#define SELECT(COND, TRUE_VAL, FALSE_VAL) ((COND) ? (TRUE_VAL) : (FALSE_VAL))

#define FLOAT2 vec2
#define FLOAT3 vec3
#define FLOAT4 vec4
#define FLOAT2x2 mat2x2
#define FLOAT2x4 mat2x4
#define UINT2 uvec2
#define UINT3 uvec3
#define UINT4 uvec4
#define USHORT2 uvec2
#define USHORT3 uvec3
#define USHORT4 uvec4
#define INT2 ivec2
#define INT3 ivec3
#define INT4 ivec4

#define PRIMID_MAX 0x7FFFFFFF

#define VS_Y_FLIP 1.0f

#define PS_UV_MSK_FIX(CB) floatBitsToUint(CB.uv_min_max)
#define PS_DISCARD discard
#define PS_SAMPLE_TEX(STATE, ...) texture(Texture, __VA_ARGS__)
#define PS_SAMPLE_TEX_LOD(STATE, ...) textureLod(Texture, __VA_ARGS__)
#define PS_SAMPLE_TEX_DEPTH(STATE, ...) PS_SAMPLE_TEX(STATE, __VA_ARGS__)
#define PS_SAMPLE_TEX_DEPTH_LOD(STATE, ...) PS_SAMPLE_TEX_LOD(STATE, __VA_ARGS__)
#define PS_READ_TEX(STATE, POS, LOD) texelFetch(Texture, (POS), (LOD))
#define PS_READ_TEX_DEPTH(STATE, POS, LOD) PS_READ_TEX(STATE, (POS), (LOD))
#define PS_READ_PALETTE(STATE, IDX) texelFetch(Palette, (POS), (LOD))
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

#ifndef VS_TME
#define VS_TME 1
#define VS_FST 1
#endif

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 3
#define GS_POINT 0
#define GS_LINE 0
#endif

#define LOAD_VERTEX(VERTICES, VID) load_vertex(VID)
#define LOAD_INDEX(INDICES, VID) load_index(VID)

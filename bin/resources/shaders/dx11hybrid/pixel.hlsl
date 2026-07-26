
cbuffer cb0 : register(b0)
{
	uint Selector0;
	uint Selector1;
	uint Selector2;
	uint Selector3;
};

cbuffer cb1 : register(b1)
{
	float2 frame_size;
};

struct PS_INPUT
{
	noperspective centroid float4 p : SV_Position;
	noperspective centroid float4 t : TEXCOORD0;
	noperspective centroid float4 c : COLOR0;
	noperspective centroid float  f : COLOR1;
};

RasterizerOrderedTexture2D<unorm float4> RT      : register(u0);
RasterizerOrderedTexture2D<float>        Depth   : register(u1);
RWTexture2D<float4>                      Texture : register(u2);
RWTexture2D<float4>                      Palette : register(u3);

static int2 curr_xy = 0; // Current pixel coordinates.
static bool discard_c = false; // Color is discarded?
static bool discard_d = false; // Depth is discarded?
static float4 curr_c = 0.0f; // Current RT color.
static float curr_z = 0.0f; // Current DS depth.
static float4 input_c = 0.0f; // Interpolated color.
static float input_z = 0.0f; // Interpolated depth.

void discard_color()
{
	discard_c = true;
}

void discard_depth()
{
	discard_d = true;
}

void discard_both()
{
	discard_c = true;
	discard_d = true;
}

void init_values(PS_INPUT input)
{
	curr_xy = int2(input.p.xy);

	if (HAS_RT)
		curr_c = RT[curr_xy];

	if (HAS_DEPTH)
		curr_z = Depth[curr_xy];

	// Truncate Z to nearest integer to emulate PS2 integer depth.
	input_z = floor(input.p.z * exp2(32.0f)) * exp2(-32.0f);

	input_c = input.c;
}

void do_z_test()
{
	if (ZTST_GEQUAL)
	{
		if (input_z < curr_z)
			discard_both();
	}

	if (ZTST_GREATER)
	{
		if (input_z <= curr_z)
			discard_both();
	}
}

void do_blend()
{
	
}

void write_back_values(float4 c, float z)
{
	if (HAS_RT && !discard_c)
		RT[curr_xy] = trunc(c) / 255.0f;

	if (HAS_DEPTH && !discard_d)
		Depth[curr_xy] = z;
}

void main(PS_INPUT input)
{
	init_values(input);
	
	do_z_test();

	do_blend();

	write_back_values(input_c, input_z);
}
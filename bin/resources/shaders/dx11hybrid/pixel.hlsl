
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

static bool discard_c = false; // Color is discarded?
static bool discard_d = false; // Depth is discarded?

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

void main(PS_INPUT input)
{
  int2 input_xy = int2(input.p.xy);
  
  // Truncate Z to nearest integer to emulate PS2 integer depth.
  float input_z = floor(input.p.z * exp2(32.0f)) * exp2(-32.0f);

  float4 input_c = input.c;
  
  float curr_z = 0.0f;
  float4 curr_c = 0.0f;

  if (HAS_RT)
  {
    curr_c = RT[input_xy];
  }

  if (HAS_DEPTH)
  {
    curr_z = Depth[input_xy];
  }

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

  if (HAS_RT && !discard_c)
  {
    RT[input_xy] = input_c;
  }

  if (HAS_DEPTH && !discard_d)
  {
    Depth[input_xy] = input_z;
  }
}
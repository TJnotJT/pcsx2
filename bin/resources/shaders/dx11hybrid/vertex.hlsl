// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

//////////////////////////////////////////////////////////////////////
// Vertex Shader
//////////////////////////////////////////////////////////////////////

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

struct VS_INPUT
{
	float2 p : POSITION0; // X, Y
	uint   z : POSITION1; // Z
	float2 t : TEXCOORD0; // S, T or U, V
  float  q : TEXCOORD1; // Q
	uint4  c : COLOR0;    // R, G, B, A
	uint   f : COLOR1;    // F
};

struct VS_OUTPUT
{
	float4 p : SV_Position; // X, Y, Z
	float4 t : TEXCOORD0;   // S, T, Q or U, V
	float4 c : COLOR0;      // R, G, B, A
  float  f : COLOR1;      // F
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = float4(2.0f * (input.p / frame_size) - 1.0f, 0.0f, 1.0f);
	output.p.z = float(input.z) * exp2(-32.0f);
  output.t = float4(input.t, input.q, 0.0f);
  output.c = input.c;
  output.f = input.f.r;

	return output;
}
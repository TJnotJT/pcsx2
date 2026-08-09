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

STATIC void texture_coord_impl(IN_PARAM(VS_INPUT_TYPE, v), OUT_PARAM(VS_OUTPUT_TYPE, out) out, IN_PARAM(VSUniformsGeneric, cb))
{
	FLOAT2 uv = FLOAT2(v.uv) - cb.texture_offset;
	FLOAT2 st = v.st - cb.texture_offset;

	// Float coordinate
	out.t.xy = st;
	out.t.w = v.q;

	// Integer coordinate => normalized
	out.ti.xy = uv * cb.texture_scale;

	if (FST)
	{
		// Integer coordinate => integral
		out.ti.zw = uv;
	}
	else
	{
		// Some games uses float coordinate for post-processing effects
		out.ti.zw = st / cb.texture_scale;
	}
}

STATIC VS_OUTPUT_TYPE vs_main_impl(IN_PARAM(VS_INPUT_TYPE, v), IN_PARAM(VSUniformsGeneric, cb))
{
	constexpr float exp_min32 = 0x1p-32;
	MainVSOut out;
	// Clamp to max depth, gs doesn't wrap
	uint z = min(v.z, cb.max_depth);
	out.p.xy = FLOAT2(v.p) - FLOAT2(0.05f, 0.05f);
	out.p.xy = out.p.xy * cb.vertex_scale - cb.vertex_offset;
  out.p.y *= VS_Y_FLIP;
	out.p.w = 1.0f;
	out.p.z = float(z) * exp_min32;

	texture_coord(v, out, cb);

	if (IIP)
		out.c = v.c;
	else
		out.fc = v.c;

	out.t.z = v.f.x; // pack fog with texture

	if (VS_POINT_SIZE)
		out.point_size = cb.point_size.x;

	return out;
}

// Convert XY from NDC to GS pixel coordinates (i.e. 1.0 = 1 GS pixel).
STATIC FLOAT2 get_xy_unscaled(FLOAT2 xy, IN_PARAM(CB_VS, cb))
{
	return round(xy / cb.vertex_scale) / 16.0f;
}

// Get the XY deltas in GS pixel coordinates, using first vertex as the origin.
STATIC FLOAT2x2 get_xy_deltas_unscaled(IN_PARAM(VS_INPUT_TYPE, v0), IN_PARAM(VS_INPUT_TYPE, v1), IN_PARAM(VS_INPUT_TYPE, v2), IN_PARAM(VSUniformsGeneric, cb))
{
	FLOAT2 xy0 = get_xy_unscaled(v0.p.xy, cb);
	FLOAT2 xy1 = get_xy_unscaled(v1.p.xy, cb);
	FLOAT2 xy2 = get_xy_unscaled(v2.p.xy, cb);
	return float2x2(xy1 - xy0, xy2 - xy0);
}

// Get the AA1 outward expand direction to the edge formed by the first two vertices.
// This is up or down for shallow (X dominant) edges, and right or left for steep (Y dominant) edges.
// Similar expansion to line AA1 except instead of expanding on both sides of the line,
// expand on on the side towards the outside of the triangle.
STATIC FLOAT2 get_aa1_triangle_expand_dir(IN_PARAM(VS_INPUT_TYPE, v0), IN_PARAM(VS_INPUT_TYPE, v1), IN_PARAM(VS_INPUT_TYPE, v2), IN_PARAM(VSUniformsGeneric, cb))
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

#if defined(__METAL_VERSION__)
STATIC FLOAT2x2 get_inverse(IN_PARAM(FLOAT2x2, mat), float det)
{
	return FLOAT2x2(mat[1][1], -mat[0][1], -mat[1][0], mat[0][0]) * (1 / det);
}
#else
// FIXME: Double check if the order of operands changes for GLSL vs MSL vs HLSL.
FLOAT2x2 get_inverse(IN_PARAM(FLOAT2x2, mat), float det)
{
	return FLOAT2x2(mat[1][1], -mat[0][1], -mat[1][0], mat[0][0]) * (1 / det);
}
#endif

// Extrapolate triangle attributes from the first vertex along the given direction.
// dp_mat is derived from the input vertices, it is passed in to avoid recomputing.
STATIC void extrapolate_aa1_triangle_edge(IN_OUT_VAR(VS_OUTPUT_TYPE, v0),IN_PARAM(VS_OUTPUT_TYPE, v1), IN_PARAM(VS_OUTPUT_TYPE, v2),
	IN_PARAM(FLOAT2x2, dp_mat), FLOAT2 dp, IN_PARAM(VSUniformsGeneric, cb))
{
	// Get texture deltas
	FLOAT2x2 dt;
	if (FST)
	{
		dt = FLOAT2x2(v1.ti.zw - v0.ti.zw, v2.ti.zw - v0.ti.zw);
	}
	else
	{
		dt = FLOAT2x2(v1.t.xy - v0.t.xy, v2.t.xy - v0.t.xy);
	}

	// Get color delta if interpolating
	FLOAT2x4 dc;
	if (IIP)
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

	FLOAT2 weights = min_perp_length < 2 ? 0 : inv_dp_mat * dp;

	v0.p.xy += dp * cb.point_size; // Extrapolate position

	// Extrapolate texture coords
	if (FST)
	{
			v0.ti.zw += dt * weights;
			v0.ti.xy = v0.ti.zw * cb.texture_scale;
	}
	else
	{
		v0.t.xy += dt * weights;
		v0.ti.zw = v0.t.xy / cb.texture_scale;
		v0.t.w += dot(dq, weights);
	}

	// Extrapolate and clamp color
	if (IIP)
	{
		v0.c += dc * weights;
		v0.c = clamp(v0.c, 0, 255);
	}

	v0.p.z += dot(dz, weights); // Extrapolate depth

	v0.t.z += dot(df, weights); // Extrapolate fog
}

STATIC VS_OUTPUT_TYPE vs_expand_none_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  return vs_main_impl(load_vertex(vertices, vid), cb);
}

STATIC VS_OUTPUT_TYPE vs_expand_point_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  VS_OUTPUT_TYPE point = vs_main_impl(load_vertex(vertices, vid), cb);
  if (vid & 1)
    point.p.x += cb.point_size.x;
  if (vid & 2)
    point.p.y += cb.point_size.y;
  return point;
}

STATIC VS_OUTPUT_TYPE vs_expand_line_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  uint vid_base = vid >> 2;
  bool is_bottom = vid & 2;
  bool is_right = vid & 1;
  uint vid_other = is_bottom ? vid_base - 1 : vid_base + 1;
  VS_OUTPUT_TYPE point = vs_main_impl(load_vertex(vertices, vid_base), cb);
  VS_OUTPUT_TYPE other = vs_main_impl(load_vertex(vertices, vid_other), cb);

  // Use bottom minus top for delta regardless of which vertex we are expanding.
  FLOAT2 line_delta = is_bottom ? point.p.xy - other.p.xy : other.p.xy - point.p.xy;
  FLOAT2 line_vector = normalize(line_delta / cb.vertex_scale);
  FLOAT2 line_expand = FLOAT2(line_vector.y, -line_vector.x);

  if (VS_EXPAND_TYPE == VS_EXPAND_LINE_AA1)
    line_expand *= 2.f * cb_line_aa1_width;

  FLOAT2 line_width = (line_expand * cb.point_size) / 2;
  FLOAT2 offset = is_right ? line_width : -line_width;
  point.p.xy += offset;

  if (VS_EXPAND_TYPE == VS_EXPAND_LINE_AA1)
    point.inv_cov = is_right ? 1.f : -1.f;

  // Lines will be run as (0 1 2) (1 2 3)
  // This means that both triangles will have a point based off the top line point as their first point
  // So we don't have to do anything for !IIP

  return point;
}

STATIC VS_OUTPUT_TYPE vs_expand_sprite_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb))
{
  uint vid_base = vid >> 1;
  bool is_bottom = vid & 2;
  bool is_right = vid & 1;
  // Sprite points are always in pairs
  uint vid_lt = vid_base & ~1;
  uint vid_rb = vid_base | 1;

  MainVSOut lt = vs_main_impl(load_vertex(vertices, vid_lt), cb);
  MainVSOut rb = vs_main_impl(load_vertex(vertices, vid_rb), cb);
  MainVSOut out = rb;

  if (!is_right)
  {
    out.p.x = lt.p.x;
    out.t.x = lt.t.x;
    out.ti.xz = lt.ti.xz;
  }

  if (!is_bottom)
  {
    out.p.y = lt.p.y;
    out.t.y = lt.t.y;
    out.ti.yw = lt.ti.yw;
  }

  return out;
}

STATIC VS_OUTPUT_TYPE vs_expand_triangle_aa1_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb), INDICES_PARAM(indices))
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

  VS_OUTPUT_TYPE out;
  if (interior)
  {
    out = vs_main_impl(load_vertex(vertices, load_index(indices, 3 * prim_id + prim_offset)), cb);
    out.inv_cov = 0.f;
    out.interior = 1;
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
    bool is_outside = edge_offset & 1;

    out                     = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + (is_bottom ? i1 : i0))), cb);
    VS_OUTPUT_TYPE other    = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + (is_bottom ? i0 : i1))), cb);
    VS_OUTPUT_TYPE opposite = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + i2)), cb);

    FLOAT2x2 pos_deltas = get_xy_deltas_unscaled(out, other, opposite, cb);

    FLOAT2 expand_dir = is_outside ? get_aa1_triangle_expand_dir(out, other, opposite, cb) : 0;

    // Do actual extrapolation, or no-op if expand_dir == 0.
    extrapolate_aa1_triangle_edge(out, other, opposite, pos_deltas, expand_dir, cb);

    out.inv_cov = is_outside ? 1.0f : 0.0f; // No coverage on outside, otherwise full.

    out.interior = 0;
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

    out                     = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + i0)), cb);
    VS_OUTPUT_TYPE other    = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + (is_first_tri ? i1 : i2))), cb);
    VS_OUTPUT_TYPE opposite = vs_main_run(load_vertex(vertices, load_index(indices, 3 * prim_id + (is_first_tri ? i2 : i1))), cb);

    float2x2 pos_deltas = get_xy_deltas_unscaled(out, other, opposite, cb);

    // Get the edge expansion directions of both incident edges.
    float2 edge_expand_dir_0 = get_aa1_triangle_expand_dir(out, other, opposite, cb);
    float2 edge_expand_dir_1 = get_aa1_triangle_expand_dir(out, opposite, other, cb);

    // Check if the corner is already filled by the expanded edges.
    // This happens if the expand directions are the same.
    // If so we output a degenerate triangle at this corner.
    bool corner_filled = all(edge_expand_dir_0 == edge_expand_dir_1);

    // Nothing if corner is filled, otherwise opposite to the bisector of the corner angle.
    float2 far_corner_dir = corner_filled ? 0 : -normalize((pos_deltas[0] + pos_deltas[1]) / 2);

    // Determine the expand direction.
    float2 expand_dir = is_near_corner ? 0 :             // No extrapolation
                        is_far_corner ? far_corner_dir : // Opposite to the angle bisector of corner
                        edge_expand_dir_0;               // Standard AA1 edge expansion

    // Do the actual extrapolation (no-op if expand_dir == 0).
    extrapolate_aa1_triangle_edge(out, other, opposite, pos_deltas, expand_dir, cb);

    out.inv_cov = is_near_corner ? 0.0f : 1.0f; // Full coverage at near corner, otherwise none.
  
    out.interior = 0;
  }
}

STATIC VS_OUTPUT_TYPE vs_expand_impl(uint vid, VERTICES_PARAM(vertices), IN_PARAM(VSUniformsGeneric, cb), INDICES_PARAM(indices))
{
	switch (VS_EXPAND_TYPE)
	{
		case VSExpand::None:
			return vs_expand_none_impl(vid, vertices, cb);
		case VSExpand::Point:
      return vs_expand_point_impl(vid, vertices, cb);
		case VSExpand::Line:
		case VSExpand::LineAA1:
      return vs_expand_line_impl(vid, vertices, cb);
		case VSExpand::Sprite:
      return vs_expand_sprite_impl(vid, vertices, cb);
		case VSExpand::TriangleAA1:
      return vs_expand_triangle_aa1_impl(vid, vertices, cb, indices);
	}
}

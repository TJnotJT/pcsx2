STATIC void discard_all(IN_OUT_PARAM(PSMainState, state))
{
	if (PS_ROV_COLOR != FALSE || PS_ROV_DEPTH != PS_ROV_DEPTH_NONE)
		state.color_discarded = state.depth_discarded = true;
	else
		GPU_DISCARD;
}

STATIC void discard_color(IN_OUT_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, color))
{
	if (PS_ROV_COLOR != FALSE)
		state.color_discarded = true;
	else
		color = state.current_color;
}

STATIC void discard_depth(IN_OUT_PARAM(PSMainState, state), IN_OUT_PARAM(float, depth))
{
	if (PS_ROV_DEPTH == PS_ROV_DEPTH_READ_WRITE)
		state.depth_discarded = true;
	else
		depth = state.current_depth;
}

FLOAT4 sample_tex_lod(IN_PARAM(PSMainState, state), FLOAT2 uv, float lod)
{
	if (PS_TEX_IS_DEPTH)
		return FLOAT4(PS_SAMPLE_TEX_DEPTH_LOD(state, uv, lod), 0.0f, 0.0f, 0.0f);
	else
		return PS_SAMPLE_TEX_LOD(state, uv, lod);
}

FLOAT4 sample_tex(IN_PARAM(PSMainState, state), FLOAT2 uv)
{
	if (PS_TEX_IS_DEPTH)
		return FLOAT4(PS_SAMPLE_TEX_DEPTH(state, uv), 0.0f, 0.0f, 0.0);
	else
		return PS_SAMPLE_TEX(state, uv);
}

FLOAT4 read_tex(IN_PARAM(PSMainState, state), UINT2 pos, uint lod)
{
	if (PS_TEX_IS_DEPTH)
		return FLOAT4(PS_READ_TEX_DEPTH(state, pos, lod), 0.0f, 0.0f, 0.0f);
	else
		return PS_READ_TEX(state, pos, lod);
}

FLOAT4 read_tex(IN_PARAM(PSMainState, state), UINT2 pos)
{
	return read_tex(state, pos, 0);
}

UINT2 get_tex_dims(IN_PARAM(PSMainState, state))
{
	UINT2 dims = UINT2(0, 0);
	if (PS_TEX_IS_DEPTH)
		PS_GET_TEX_DEPTH_DIMS(state, dims);
	else
		PS_GET_TEX_DIMS(state, dims);
	return dims;
}

STATIC float manual_lod(IN_PARAM(PSMainState, state), float uv_w)
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

STATIC FLOAT4 sample_c_af(IN_PARAM(PSMainState, state), FLOAT2 uv, float uv_w)
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

		float denom_plus  = t * (q + t);
		float denom_minus = t * (q - t);

		float sqrtA = sqrt(F * (t + p));
		float sqrtB = sqrt(F * (t - p));

		float inv_sqrt_denom_plus  = RSQRT(denom_plus);
		float inv_sqrt_denom_minus = RSQRT(denom_minus);

		float signB = sign(B);

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
		colour = sample_tex_lod(state, uv, lod);
	}
	else
	{
		FLOAT4 num = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		FLOAT2 segment = (2.0f * aniso_line) / aniso_ratio;
		for (int i = 0; i < aniso_ratio; i++)
		{
			FLOAT2 d = -aniso_line + (0.5f + i) * segment;
			FLOAT2 uv_sample = uv + d;
			FLOAT4 sample_colour = sample_tex_lod(state, uv_sample, lod);
			num += sample_colour;
		}

		colour = num / aniso_ratio;
	}
	return colour;
}

STATIC FLOAT4 sample_c(IN_PARAM(PSMainState, state), FLOAT2 uv)
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
		return sample_c_af(state, uv, state.psin.t.w);
	else if (PS_AUTOMATIC_LOD != FALSE)
		return sample_tex(state, uv);
	else if (PS_MANUAL_LOD != FALSE)
		return sample_tex_lod(state, uv, manual_lod(state, state.psin.t.w));
	else
		return sample_tex_lod(state, uv, 0);
}

FLOAT4 sample_p(IN_PARAM(PSMainState, state), uint idx)
{
	return PS_READ_PALETTE(state, UINT2(idx, 0));
}

FLOAT4 sample_p_norm(IN_PARAM(PSMainState, state), float u)
{
	return sample_p(state, uint(u * 255.5f));
}

FLOAT4 clamp_wrap_uv(IN_PARAM(PSMainState, state), FLOAT4 uv)
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

FLOAT4x4 sample_4c(IN_PARAM(PSMainState, state), FLOAT4 uv)
{
	return FLOAT4x4(
		sample_c(state, uv.xy),
		sample_c(state, uv.zy),
		sample_c(state, uv.xw),
		sample_c(state, uv.zw)
	);
}

UINT4 sample_4_index(IN_PARAM(PSMainState, state), FLOAT4 uv)
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

FLOAT4x4 sample_4p(IN_PARAM(PSMainState, state), UINT4 u)
{
	return FLOAT4x4(
		sample_p(state, u.x),
		sample_p(state, u.y),
		sample_p(state, u.z),
		sample_p(state, u.w)
	);
}

uint fetch_raw_depth(IN_PARAM(PSMainState, state))
{
	return uint(PS_READ_TEX_DEPTH(state, USHORT2(state.psin.p.xy + state.cb.channel_shuffle_offset), 0) * EXP2_POS_32);
}

FLOAT4 fetch_raw_color(IN_PARAM(PSMainState, state))
{
	if (PS_TEX_IS_FB != FALSE)
		return state.current_color;
	else
		return PS_READ_TEX(state, USHORT2(state.psin.p.xy + state.cb.channel_shuffle_offset), 0);
}

FLOAT4 fetch_c(IN_PARAM(PSMainState, state), USHORT2 uv)
{
	if (PS_TEX_IS_FB != FALSE)
		return state.current_color;
	else if (PS_TEX_IS_DEPTH)
		return FLOAT4(PS_READ_TEX_DEPTH(state, uv, 0), 0.0f, 0.0f, 0.0f);
	else
		return PS_READ_TEX(state, uv, 0);
}

USHORT2 clamp_wrap_uv_depth(IN_PARAM(PSMainState, state), USHORT2 uv)
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

FLOAT4 sample_depth(IN_PARAM(PSMainState, state), FLOAT2 st)
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
		t.a = (PS_AEM == FALSE || any_nonzero(t.rgb)) ? 255.f * state.cb.ta.x : 0.f;
	else if (PS_AEM_FMT == FMT_16)
		t.a = t.a >= 128.f ? 255.f * state.cb.ta.y : (PS_AEM == FALSE || any_nonzero(t.rgb)) ? 255.f * state.cb.ta.x : 0.f;
	else if (PS_PAL_FMT != 0 && PS_TALES_OF_ABYSS_HLE == FALSE && PS_URBAN_CHAOS_HLE == FALSE)
		t = trunc(sample_4p(state, UINT4(t.aaaa))[0] * 255.0f + 0.05f);
	
	return t;
}

// MARK: Fetch a Single Channel

FLOAT4 fetch_red(IN_PARAM(PSMainState, state))
{
	float rt = PS_TEX_IS_DEPTH ? float(fetch_raw_depth(state) & 0xFFu) / 255.f : fetch_raw_color(state).r;
	return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_green(IN_PARAM(PSMainState, state))
{
	float rt = PS_TEX_IS_DEPTH ? float((fetch_raw_depth(state) >> 8) & 0xFFu) / 255.f : fetch_raw_color(state).g;
	return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_blue(IN_PARAM(PSMainState, state))
{
	float rt = PS_TEX_IS_DEPTH ? float((fetch_raw_depth(state) >> 16) & 0xFFu) / 255.f : fetch_raw_color(state).b;
	return sample_p_norm(state, rt) * 255.f;
}

FLOAT4 fetch_alpha(IN_PARAM(PSMainState, state))
{
	return sample_p_norm(state, fetch_raw_color(state).a) * 255.f;
}

FLOAT4 fetch_rgb(IN_PARAM(PSMainState, state))
{
	FLOAT4 rt = fetch_raw_color(state);
	return FLOAT4(sample_p_norm(state, rt.r).r, sample_p_norm(state, rt.g).g, sample_p_norm(state, rt.b).b, 1) * 255.f;
}

FLOAT4 fetch_gXbY(IN_PARAM(PSMainState, state))
{
	if (PS_TEX_IS_DEPTH)
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

FLOAT4 sample_color(IN_PARAM(PSMainState, state), FLOAT2 st)
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
			c[i].a = (PS_AEM == FALSE || any_nonzero(c[i].rgb)) ? state.cb.ta.x : 0.f;
		else if (PS_AEM_FMT == FMT_16)
			c[i].a = c[i].a >= 0.5 ? state.cb.ta.y : PS_AEM == FALSE || any_nonzero(INT3(c[i].rgb * 255.0f) & int(0xF8)) ? state.cb.ta.x : 0.f;
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

FLOAT4 tfx(IN_PARAM(PSMainState, state), FLOAT4 T, FLOAT4 C)
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

bool atst(IN_PARAM(PSMainState, state), FLOAT4 C)
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

void fog(IN_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, C), float f)
{
	if (PS_FOG != FALSE)
		C.rgb = trunc(MIX(state.cb.fog_color, C.rgb, (f * 255.0f) / 256.0f));
}

FLOAT4 ps_color(IN_PARAM(PSMainState, state))
{
	FLOAT2 st, st_int;
	if (PS_FST == FALSE)
	{
		st = state.psin.t.xy / state.psin.t.w;
		st_int = state.psin.ti.zw / state.psin.t.w;
	}
	else
	{
		// Note: xy are normalized coordinates
		st = state.psin.ti.xy;
		st_int = state.psin.ti.zw;
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
		
		T.a = (T.a >= 127.5 ? state.cb.ta.y : PS_AEM == FALSE || any_nonzero(INT3(T.rgb) & int(0xF8)) ? state.cb.ta.x : 0.f) * 255.f;
	}

	FLOAT4 C = tfx(state, T, PS_IIP != FALSE ? state.psin.c : state.psin.fc);

	fog(state, C, state.psin.t.z);

	return C;
}

void ps_fbmask(IN_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, C))
{
	if (PS_FBMASK != FALSE)
	{
		float multi = (PS_COLCLIP_HW != FALSE) ? 65535.0 : 255.5;
		C = FLOAT4((UINT4(INT4(C)) & (state.cb.fbmask ^ 0xffu)) | (UINT4(state.current_color * FLOAT4(multi, multi, multi, 255)) & state.cb.fbmask));
	}
}

void ps_dither(IN_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, C), float As)
{
	if (PS_DITHER == 0 || PS_DITHER == 3)
		return;
	USHORT2 fpos;
	if (PS_DITHER == 2)
		fpos = USHORT2(state.psin.p.xy);
	else
		fpos = USHORT2(state.psin.p.xy * float2_bcast(state.cb.scale_factor.y));
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

void ps_color_clamp_wrap(IN_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, C))
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
			C.rgb = FLOAT3(SHORT3(C.rgb) & SHORT(0xF8));
		else if (PS_COLCLIP == 1 || PS_COLCLIP_HW == 1)
			C.rgb = FLOAT3(SHORT3(C.rgb) & 0xFF);
	}
	else if (PS_DST_FMT == FMT_16 && PS_DITHER != 3 && PS_BLEND_MIX == 0 && PS_BLEND_HW == 0)
		C.rgb = FLOAT3(SHORT3(C.rgb) & SHORT(0xF8));
}

#define PICK3(SELECTOR, ZERO, ONE, TWO) ((SELECTOR) == 0 ? (ZERO) : (SELECTOR) == 1 ? (ONE) : (TWO))

void ps_blend(IN_OUT_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, Color), IN_OUT_PARAM(FLOAT4, As_rgba))
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

void ps_shuffle(IN_PARAM(PSMainState, state), IN_OUT_PARAM(FLOAT4, C))
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

PSOutputGeneric ps_main_impl(IN_OUT_PARAM(PSMainState, state))
{
	PSOutputGeneric psout;
	psout.c0 = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	psout.c1 = FLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	psout.depth = 0.0f;

	float input_z = state.psin.p.z;
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
		if ((uint(state.psin.p.y) & 1u) == (uint(PS_SCANMSK) & 1u))
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
		float stencil_ceil = PS_READ_PRIMID(state, UINT2(state.psin.p.xy));
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
			? SATURATE(state.cb.line_cov_scale * (1.f - abs(state.psin.inv_cov))) // Blur only outer part of the line by scaling coverage.
			: SATURATE(1.f - abs(state.psin.inv_cov));
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
		input_z = min(input_z, state.cb.max_depth_ps);

	if (PS_AA1 == PS_AA1_TRIANGLE_SW_Z && state.psin.interior == 0)
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
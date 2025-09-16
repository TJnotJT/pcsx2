// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

// TODO: JIT Draw* (flags: depth, texture, color (+iip), scissor)

#include "GS/Renderers/SW/GSRasterizer.h"
#include "GS/Renderers/SW/GSDrawScanline.h"
#include "GS/GSExtra.h"
#include "PerformanceMetrics.h"
#include "VMManager.h"

#include "common/AlignedMalloc.h"
#include "common/Console.h"
#include "common/StringUtil.h"

#define ENABLE_DRAW_STATS 0

MULTI_ISA_UNSHARED_IMPL;

int GSRasterizerData::s_counter = 0;

static int compute_best_thread_height(int threads)
{
	// - for more threads screen segments should be smaller to better distribute the pixels
	// - but not too small to keep the threading overhead low
	// - ideal value between 3 and 5, or log2(64 / number of threads)

	int th = GSConfig.SWExtraThreadsHeight;

	if (th > 0 && th < 9)
		return th;
	else
		return 4;
}

GSRasterizer::GSRasterizer(GSDrawScanline* ds, int id, int threads)
	: m_ds(ds)
	, m_id(id)
	, m_threads(threads)
	, m_scanmsk_value(0)
{
	memset(&m_pixels, 0, sizeof(m_pixels));
	m_primcount = 0;

	m_thread_height = compute_best_thread_height(threads);

	m_edge.buff = static_cast<GSVertexSW*>(_aligned_malloc(sizeof(GSVertexSW) * 2048, VECTOR_ALIGNMENT));
	m_edge.count = 0;
	if (!m_edge.buff)
		pxFailRel("failed to allocate storage for m_edge.buff");

	int rows = (2048 >> m_thread_height) + 16;
	m_scanline = (u8*)_aligned_malloc(rows, 64);

	for (int i = 0; i < rows; i++)
	{
		m_scanline[i] = (i % threads) == id ? 1 : 0;
	}
}

GSRasterizer::~GSRasterizer()
{
	_aligned_free(m_scanline);
	_aligned_free(m_edge.buff);
}

static void __fi AddScanlineInfo(GSVertexSW* e, int pixels, int left, int top)
{
	e->_pad.I32[0] = pixels;
	e->_pad.I32[1] = left;
	e->_pad.I32[2] = top;
}

bool GSRasterizer::IsOneOfMyScanlines(int top) const
{
	pxAssert(top >= 0 && top < 2048);

	return m_scanline[top >> m_thread_height] != 0;
}

bool GSRasterizer::IsOneOfMyScanlines(int top, int bottom) const
{
	pxAssert(top >= 0 && top < 2048 && bottom >= 0 && bottom < 2048);

	top = top >> m_thread_height;
	bottom = (bottom + (1 << m_thread_height) - 1) >> m_thread_height;

	while (top < bottom)
	{
		if (m_scanline[top++])
		{
			return true;
		}
	}

	return false;
}

int GSRasterizer::FindMyNextScanline(int top) const
{
	int i = top >> m_thread_height;

	if (m_scanline[i] == 0)
	{
		while (m_scanline[++i] == 0)
			;

		top = i << m_thread_height;
	}

	return top;
}

int GSRasterizer::GetPixels(bool reset)
{
	int pixels = m_pixels.sum;

	if (reset)
	{
		m_pixels.sum = 0;
	}

	return pixels;
}

void GSRasterizer::Draw(GSRasterizerData& data)
{
	if ((data.vertex && data.vertex_count == 0) || (data.index && data.index_count == 0))
		return;

	m_pixels.actual = 0;
	m_pixels.total = 0;
	m_primcount = 0;

	if constexpr (ENABLE_DRAW_STATS)
		data.start = GetCPUTicks();

	m_setup_prim = data.setup_prim;
	m_draw_scanline = data.draw_scanline;
	m_draw_edge = data.draw_edge;
	GSDrawScanline::BeginDraw(data, m_local);

	const GSVertexSW* vertex = data.vertex;
	const GSVertexSW* vertex_end = data.vertex + data.vertex_count;

	const u16* index = data.index;
	const u16* index_end = data.index + data.index_count;

	static constexpr u16 tmp_index[] = {0, 1, 2};

	bool scissor_test = !data.bbox.eq(data.bbox.rintersect(data.scissor));

	m_scissor = data.scissor;
	m_fscissor_x = GSVector4(data.scissor).xzxz();
	m_fscissor_y = GSVector4(data.scissor).ywyw();
	m_scanmsk_value = data.scanmsk_value;

	switch (data.primclass)
	{
		case GS_POINT_CLASS:

			if (scissor_test)
			{
				DrawPoint<true>(vertex, data.vertex_count, index, data.index_count);
			}
			else
			{
				DrawPoint<false>(vertex, data.vertex_count, index, data.index_count);
			}

			break;

		case GS_LINE_CLASS:

			if (index != NULL)
			{
				do
				{
					DrawLine(vertex, index);
					index += 2;
				} while (index < index_end);
			}
			else
			{
				do
				{
					DrawLine(vertex, tmp_index);
					vertex += 2;
				} while (vertex < vertex_end);
			}

			break;

		case GS_TRIANGLE_CLASS:

			if (index != NULL)
			{
				do
				{
					DrawTriangle(vertex, index);
					index += 3;
				} while (index < index_end);
			}
			else
			{
				do
				{
					DrawTriangle(vertex, tmp_index);
					vertex += 3;
				} while (vertex < vertex_end);
			}

			break;

		case GS_SPRITE_CLASS:

			if (index != NULL)
			{
				do
				{
					DrawSprite(vertex, index);
					index += 2;
				} while (index < index_end);
			}
			else
			{
				do
				{
					DrawSprite(vertex, tmp_index);
					vertex += 2;
				} while (vertex < vertex_end);
			}

			break;

		default:
			ASSUME(0);
	}

#if _M_SSE >= 0x501
	_mm256_zeroupper();
#endif

	data.pixels = m_pixels.actual;

	m_pixels.sum += m_pixels.actual;

	if constexpr (ENABLE_DRAW_STATS)
		m_ds->UpdateDrawStats(data.frame, GetCPUTicks() - data.start, m_pixels.actual, m_pixels.total, m_primcount);
}

template <bool scissor_test>
void GSRasterizer::DrawPoint(const GSVertexSW* vertex, int vertex_count, const u16* index, int index_count)
{
	m_primcount++;

	if (index)
	{
		for (int i = 0; i < index_count; i++, index++)
		{
			const GSVertexSW& v = vertex[*index];

			GSVector4i p(v.p);

			if (!scissor_test || (m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom))
			{
				if (IsOneOfMyScanlines(p.y))
				{
					m_setup_prim(vertex, index, GSVertexSW::zero(), m_local);

					DrawScanline(1, p.x, p.y, v);
				}
			}
		}
	}
	else
	{
		static constexpr u16 tmp_index[1] = {0};

		for (int i = 0; i < vertex_count; i++, vertex++)
		{
			const GSVertexSW& v = vertex[0];

			GSVector4i p(v.p);

			if (!scissor_test || (m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom))
			{
				if (IsOneOfMyScanlines(p.y))
				{
					m_setup_prim(vertex, tmp_index, GSVertexSW::zero(), m_local);

					DrawScanline(1, p.x, p.y, v);
				}
			}
		}
	}
}

#if 0
template<bool step_x, bool pos_x, bool pos_y>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{

	GSVector4 dp = dv.p;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;

	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	if (x0 == 173.3750 && y0 == 266.2500)
	{
		printf("");
	}


	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	float slope = step_x ? ((y1 - y0) / (x1 - x0)) : ((x1 - x0) / (y1 - y0));

	GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));

	float x = step_x ? rx0 : x0;
	float y = step_x ? y0 : ry0;

	float dx = step_x ? (pos_x ? 1.0f : -1.0f) : slope;
	float dy = step_x ? slope : (pos_y ? 1.0f : -1.0f);

	dx *= step_x ? 1.0f : dy;
	dy *= step_x ? dx : 1.0f;

	GSVertexSW edge(v0);

	edge += dedge * GSVector4(step_x ? (x - x0) : (y - y0));
	x += step_x ? 0.0f : slope * (y - y0);
	y += step_x ? slope * (x - x0) : 0.0f;

	// Diamond exit rule that GS uses for determining line coverage.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(x0 - rx0, y0 - ry0);
	bool draw_last = TestRegion(x1 - rx1, y1 - ry1);

	GSVertexSW* RESTRICT e = m_edge.buff;

	while (true)
	{
		int xi = step_x ? static_cast<int>(x) : static_cast<int>(std::floor(x + 0.5f));
		int yi = step_x ? static_cast<int>(std::floor(y + 0.5f)) : static_cast<int>(y);

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		if (step_x ? (x == rx0) : (y == ry0))
			draw = draw && draw_first;
		if (step_x ? (x == rx1) : (y == ry1))
			draw = draw && draw_last;

		if  (draw)
		{
			AddScanline(e, 1, xi, yi, edge);

			e++;
		}

		if (step_x ? (x == rx1) : (y == ry1))
			break;

		edge += dedge;
		x += dx;
		y += dy;
	}

	m_edge.count = e - m_edge.buff;
}
#endif

#if 0
// Floating point decision
template <bool step_x, bool pos_x, bool pos_y>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{

	GSVector4 dp = dv.p;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;

	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	if (x0 == 173.3750 && y0 == 266.2500)
	{
		printf("");
	}

	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	float slope = step_x ? ((y1 - y0) / (x1 - x0)) : ((x1 - x0) / (y1 - y0));

	GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));

	const int delta_x = static_cast<int>(16.0f * (x1 - x0));
	const int delta_y = static_cast<int>(16.0f * (y1 - y0));

	int slope_scale = static_cast<int>(16.0f * (step_x ? (x1 - x0) : (y1 - y0)));

	float x = step_x ? rx0 : x0;
	float y = step_x ? y0 : ry0;

	float dx = step_x ? (pos_x ? 1.0f : -1.0f) : slope;
	float dy = step_x ? slope : (pos_y ? 1.0f : -1.0f);

	dx *= step_x ? 1.0f : dy;
	dy *= step_x ? dx : 1.0f;

	float decision = (step_x ? y - ry0 : x - rx0) - 0.5f;
	float step_decision = step_x ? dy : dx;

	GSVertexSW edge(v0);
	int xi = static_cast<int>(rx0);
	int yi = static_cast<int>(ry0);

	edge += dedge * GSVector4(step_x ? (x - x0) : (y - y0));
	x += step_x ? 0.0f : slope * (y - y0);
	y += step_x ? slope * (x - x0) : 0.0f;
	decision += slope * (step_x ? (x - x0) : (y - y0)); 

	// Diamond exit rule that GS uses for determining line coverage.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(x0 - rx0, y0 - ry0);
	bool draw_last = TestRegion(x1 - rx1, y1 - ry1);

	GSVertexSW* RESTRICT e = m_edge.buff;

	while (true)
	{
		int xi = step_x ? static_cast<int>(x) : static_cast<int>(std::floor(x + 0.5f));
		int yi = step_x ? static_cast<int>(std::floor(y + 0.5f)) : static_cast<int>(y);

		float tmp = step_x ? (y - static_cast<float>(yi) - 0.5f) : (x - static_cast<float>(xi) - 0.5f);
		if (std::abs(decision - tmp) > 0.001f)
		{
			printf("");
		}

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		if (step_x ? (x == rx0) : (y == ry0))
			draw = draw && draw_first;
		if (step_x ? (x == rx1) : (y == ry1))
			draw = draw && draw_last;

		if (draw)
		{
			AddScanline(e, 1, xi, yi, edge);

			e++;
		}

		if (step_x ? (x == rx1) : (y == ry1))
			break;

		edge += dedge;
		x += dx;
		y += dy;
		decision += step_decision;
		if (decision >= 0.0f)
			decision -= 1.0f;
		if (decision < -1.0f)
			decision += 1.0f;
	}

	m_edge.count = e - m_edge.buff;
}
#endif

#if 0
// Scaled decision variable
template <bool step_x, bool pos_x, bool pos_y>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{
	GSVector4 dp = dv.p;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;

	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	if (x0 == 173.3750 && y0 == 266.2500)
	{
		printf("");
	}

	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	float slope = step_x ? ((y1 - y0) / (x1 - x0)) : ((x1 - x0) / (y1 - y0));

	GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));

	const int delta_x = static_cast<int>(16.0f * (x1 - x0));
	const int delta_y = static_cast<int>(16.0f * (y1 - y0));

	int slope_scale = static_cast<int>(16.0f * (step_x ? (x1 - x0) : (y1 - y0)));

	float x = step_x ? rx0 : x0;
	float y = step_x ? y0 : ry0;

	float dx = step_x ? (pos_x ? 1.0f : -1.0f) : slope;
	float dy = step_x ? slope : (pos_y ? 1.0f : -1.0f);

	dx *= step_x ? 1.0f : dy;
	dy *= step_x ? dx : 1.0f;

	float decision_scale = std::abs(2.0f * (step_x ? (x1 - x0) : (y1 - y0)));
	float decision = (decision_scale) * ((step_x ? y - ry0 : x - rx0) - 0.5f);
	float step_decision = (decision_scale) * (step_x ? dy : dx);

	GSVertexSW edge(v0);
	int xi = static_cast<int>(rx0);
	int yi = static_cast<int>(ry0);

	edge += dedge * GSVector4(step_x ? (x - x0) : (y - y0));
	x += step_x ? 0.0f : slope * (y - y0);
	y += step_x ? slope * (x - x0) : 0.0f;
	decision += decision_scale * slope * (step_x ? (x - x0) : (y - y0));

	// Diamond exit rule that GS uses for determining line coverage.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(x0 - rx0, y0 - ry0);
	bool draw_last = TestRegion(x1 - rx1, y1 - ry1);

	GSVertexSW* RESTRICT e = m_edge.buff;

	if (decision >= 0.0f)
		decision -= (decision_scale);
	if (decision < -decision_scale)
		decision += (decision_scale);

	while (true)
	{
		int xi = step_x ? static_cast<int>(x) : static_cast<int>(std::floor(x + 0.5f));
		int yi = step_x ? static_cast<int>(std::floor(y + 0.5f)) : static_cast<int>(y);

		float tmp = (decision_scale) * (step_x ? (y - static_cast<float>(yi) - 0.5f) : (x - static_cast<float>(xi) - 0.5f));
		if (std::abs(decision - tmp) > 0.1f && std::abs(decision + decision_scale - tmp) > 0.1f)
		{
			printf("");
		}

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		if (step_x ? (x == rx0) : (y == ry0))
			draw = draw && draw_first;
		if (step_x ? (x == rx1) : (y == ry1))
			draw = draw && draw_last;

		if (draw)
		{
			AddScanline(e, 1, xi, yi, edge);

			e++;
		}

		if (step_x ? (x == rx1) : (y == ry1))
			break;

		edge += dedge;
		x += dx;
		y += dy;
		decision += step_decision;
		if (decision >= 0.0f)
			decision -= (decision_scale);
		if (decision < -decision_scale)
			decision += (decision_scale);
	}

	m_edge.count = e - m_edge.buff;
}
#endif

#if 0
// ADDED THE INTEGER DECISION
template <bool step_x, bool pos_x, bool pos_y>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{
	GSVector4 dp = dv.p;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;

	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	if (x0 == 173.3750 && y0 == 266.2500)
	{
		printf("");
	}

	const auto FPToInt = [](float f) { return static_cast<int>(16.0f * f); };

	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	float slope = step_x ? ((y1 - y0) / (x1 - x0)) : ((x1 - x0) / (y1 - y0));

	GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));

	int slope_scale = static_cast<int>(16.0f * (step_x ? (x1 - x0) : (y1 - y0)));

	float x = step_x ? rx0 : x0;
	float y = step_x ? y0 : ry0;

	float dx = step_x ? (pos_x ? 1.0f : -1.0f) : slope;
	float dy = step_x ? slope : (pos_y ? 1.0f : -1.0f);

	dx *= step_x ? 1.0f : dy;
	dy *= step_x ? dx : 1.0f;

	float decision_scale = std::abs(16.0f * 32.0f * (step_x ? (x1 - x0) : (y1 - y0)));
	float decision = (decision_scale) * ((step_x ? y - ry0 : x - rx0) - 0.5f);
	float step_decision = (decision_scale) * (step_x ? dy : dx);

	const float delta_x = x1 - x0;
	const float delta_y = y1 - y0;
	int scaleD = static_cast<int>(2 * 16 * 16 * std::abs(step_x ? delta_x : delta_y));
	int D = static_cast<int>(scaleD * ((step_x ? (y0 - ry0) : (x0 - rx0)) - 0.5f));
	int dD = static_cast<int>(2 * 16 * 16 * (step_x ? delta_y : delta_x));
	int sign_step = step_x ? (pos_x ? 1 : -1) : (pos_y ? 1 : -1);

	GSVertexSW edge(v0);
	int xi = static_cast<int>(rx0);
	int yi = static_cast<int>(ry0);

	edge += dedge * GSVector4(step_x ? (x - x0) : (y - y0));
	x += step_x ? 0.0f : slope * (y - y0);
	y += step_x ? slope * (x - x0) : 0.0f;
	decision += decision_scale * slope * (step_x ? (x - x0) : (y - y0));
	D += static_cast<int>(dD * (step_x ? (rx0 - x0) : (ry0 - y0))) * sign_step; // FIXME: overflow?

	// Diamond exit rule that GS uses for determining line coverage.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(x0 - rx0, y0 - ry0);
	bool draw_last = TestRegion(x1 - rx1, y1 - ry1);

	GSVertexSW* RESTRICT e = m_edge.buff;

	if (decision >= 0.0f)
		decision -= (decision_scale);
	if (decision < -decision_scale)
		decision += (decision_scale);
	if (D >= 0)
		D -= scaleD;
	if (D < -scaleD)
		D += scaleD;

	while (true)
	{
		int xi = step_x ? static_cast<int>(x) : static_cast<int>(std::floor(x + 0.5f));
		int yi = step_x ? static_cast<int>(std::floor(y + 0.5f)) : static_cast<int>(y);

		float tmp0 = (step_x ? (y - static_cast<float>(yi) - 0.5f) : (x - static_cast<float>(xi) - 0.5f));
		float tmp1 = decision / decision_scale;
		float tmp2 = static_cast<float>(D) / static_cast<float>(scaleD);
		if (std::abs(tmp1 - tmp0) > 0.01f && std::abs(tmp1 + 1.0f - tmp0) > 0.1f)
		{
			printf("");
		}
		if (std::abs(tmp2 - tmp0) > 0.01f && std::abs(tmp2 + 1.0f - tmp0) > 0.1f)
		{
			printf("");
		}
		
		//printf("%f %f %f\n", tmp0, tmp1, tmp2);

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		if (step_x ? (x == rx0) : (y == ry0))
			draw = draw && draw_first;
		if (step_x ? (x == rx1) : (y == ry1))
			draw = draw && draw_last;

		if (draw)
		{
			AddScanline(e, 1, xi, yi, edge);

			e++;
		}

		if (step_x ? (x == rx1) : (y == ry1))
			break;

		edge += dedge;
		x += dx;
		y += dy;
		decision += step_decision;
		if (decision >= 0.0f)
			decision -= (decision_scale);
		if (decision < -decision_scale)
			decision += (decision_scale);
		D += dD;
		if (step_x ? pos_y : pos_x)
		{
			if (D >= 0)
				D -= scaleD;
		}
		else
		{
			if (D < -scaleD)
				D += scaleD;
		}
	}

	m_edge.count = e - m_edge.buff;
}
#endif

#if 0
// Results seem to match almost exactly with PS2. Now remove the FP parts.
template <bool step_x, bool pos_x, bool pos_y>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{
	GSVector4 dp = dv.p;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;

	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	if (x0 == 173.3750 && y0 == 266.2500)
	{
		printf("");
	}

	const auto FPToInt = [](float f) { return static_cast<int>(16.0f * f); };

	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	float slope = step_x ? ((y1 - y0) / (x1 - x0)) : ((x1 - x0) / (y1 - y0));

	GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));

	int slope_scale = static_cast<int>(16.0f * (step_x ? (x1 - x0) : (y1 - y0)));

	float x = step_x ? rx0 : x0;
	float y = step_x ? y0 : ry0;

	float dx = step_x ? (pos_x ? 1.0f : -1.0f) : slope;
	float dy = step_x ? slope : (pos_y ? 1.0f : -1.0f);

	dx *= step_x ? 1.0f : dy;
	dy *= step_x ? dx : 1.0f;

	const float delta_x = x1 - x0;
	const float delta_y = y1 - y0;
	int scaleD = static_cast<int>(2 * 16 * 16 * std::abs(step_x ? delta_x : delta_y));
	int D = static_cast<int>(scaleD * ((step_x ? (y0 - ry0) : (x0 - rx0)) - 0.5f));
	int dD = static_cast<int>(2 * 16 * 16 * (step_x ? delta_y : delta_x));
	int sign_step = step_x ? (pos_x ? 1 : -1) : (pos_y ? 1 : -1);

	GSVertexSW edge(v0);
	

	edge += dedge * GSVector4(step_x ? (x - x0) : (y - y0));
	x += step_x ? 0.0f : slope * (y - y0);
	y += step_x ? slope * (x - x0) : 0.0f;
	D += static_cast<int>(dD * (step_x ? (rx0 - x0) : (ry0 - y0))) * sign_step; // FIXME: overflow?

	// Diamond exit rule that GS uses for determining line coverage.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(x0 - rx0, y0 - ry0);
	bool draw_last = TestRegion(x1 - rx1, y1 - ry1);

	GSVertexSW* RESTRICT e = m_edge.buff;

	constexpr int dxi = pos_x ? 1 : -1;
	constexpr int dyi = pos_y ? 1 : -1;
	int xi2 = static_cast<int>(rx0);
	int yi2 = static_cast<int>(ry0);


	if (D >= 0)
	{
		D -= scaleD;
		xi2 += step_x ? 0 : 1;
		yi2 += step_x ? 1 : 0;
	}
	else if (D < -scaleD)
	{
		D += scaleD;
		xi2 += step_x ? 0 : -1;
		yi2 += step_x ? -1 : 0;
	}

	pxAssert(-scaleD <= D && D <= 0);

	while (true)
	{
		int xi = step_x ? static_cast<int>(x) : static_cast<int>(std::floor(x + 0.5f));
		int yi = step_x ? static_cast<int>(std::floor(y + 0.5f)) : static_cast<int>(y);

		if (xi != xi2 || yi != yi2)
		{
			printf("");
		}

		float tmp0 = (step_x ? (y - static_cast<float>(yi) - 0.5f) : (x - static_cast<float>(xi) - 0.5f));
		float tmp1 = static_cast<float>(D) / static_cast<float>(scaleD);
		if (std::abs(tmp1 - tmp0) > 0.01f && std::abs(tmp1 + 1.0f - tmp0) > 0.1f)
		{
			printf("");
		}

		//printf("%f %f %f\n", tmp0, tmp1);

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		if (step_x ? (x == rx0) : (y == ry0))
			draw = draw && draw_first;
		if (step_x ? (x == rx1) : (y == ry1))
			draw = draw && draw_last;

		if (draw)
		{
			AddScanline(e, 1, xi, yi, edge);

			e++;
		}

		if (step_x ? (x == rx1) : (y == ry1))
			break;

		edge += dedge;
		x += dx;
		y += dy;
		D += dD;
		xi2 += step_x ? dxi : 0;
		yi2 += step_x ? 0 : dyi;
		if (step_x ? pos_y : pos_x)
		{
			if (D >= 0)
			{
				D -= scaleD;
				xi2 += step_x ? 0 : 1;
				yi2 += step_x ? 1 : 0;
			}
		}
		else
		{
			if (D < -scaleD)
			{
				D += scaleD;
				xi2 += step_x ? 0 : -1;
				yi2 += step_x ? -1 : 0;
			}
		}
	}

	m_edge.count = e - m_edge.buff;
}
#endif

template <bool step_x, bool pos_x, bool pos_y, bool has_edge>
void GSRasterizer::DrawLineImpl(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv)
{
	constexpr int dxi = pos_x ? 1 : -1;
	constexpr int dyi = pos_y ? 1 : -1;

	const float delta_x = dv.p.x;
	const float delta_y = dv.p.y;

	if (delta_x == 0.0f && delta_y == 0.0f)
		return;

	const float x0 = v0.p.x;
	const float y0 = v0.p.y;
	const float x1 = v1.p.x;
	const float y1 = v1.p.y;

	const float rx0 = std::floor(x0 + 0.5f);
	const float ry0 = std::floor(y0 + 0.5f);
	const float rx1 = std::floor(x1 + 0.5f);
	const float ry1 = std::floor(y1 + 0.5f);

	const float fx0 = x0 - rx0;
	const float fy0 = y0 - ry0;
	const float fx1 = x1 - rx1;
	const float fy1 = y1 - ry1;

	const int rxi0 = static_cast<int>(rx0);
	const int ryi0 = static_cast<int>(ry0);
	const int rxi1 = static_cast<int>(rx1);
	const int ryi1 = static_cast<int>(ry1);

	const GSVertexSW dedge = dv / GSVector4(step_x ? (x1 - x0) : (y1 - y0));
	
	GSVertexSW edge(v0);

	GSVertexSW* RESTRICT e = m_edge.buff;

	// Decision value for y when step_x == true (and vice versa).
	// D is the fractional part of y scaled and shifted. When pos_y == true, D is kept in the range [-scaleD, 0)
	// so that D >= 0 indicated a +1 step in y. When pos_y == false, D is kept in the range [0, scaleD) so that
	// D < 0 indicates a -1 step in y.
	constexpr bool pos_D = step_x ? pos_y : pos_x;
	const int scaleD = static_cast<int>(2 * 16 * 16 * std::abs(step_x ? delta_x : delta_y));
	const float scaleDf = static_cast<float>(scaleD); // Needed only if use_edge == true.
	const int dD = static_cast<int>(2 * 16 * 16 * (step_x ? delta_y : delta_x));
	int D = static_cast<int>(scaleD * ((step_x ? fy0 : fx0) + (pos_D ? -0.5f : 0.5f)));

	// Diamond exit rule for determining coverage of first/last pixel.
	const auto TestRegion = [](float dx, float dy) -> bool {
		float dist = std::abs(dx) + std::abs(dy);
		if (dist < 0.5)
			return false;
		if constexpr (step_x)
		{
			const bool x_good = pos_x ? (dx > 0) : (dx < 0);
			return x_good && (dist > 0.5 || dy >= 0);
		}
		else
		{
			const bool y_good = pos_y ? (dy > 0) : (dy < 0);
			return y_good && (dist > 0.5 || dx >= 0);
		}
	};

	bool draw_first = !TestRegion(fx0, fy0);
	bool draw_last = TestRegion(fx1, fy1);

	int xi = rxi0;
	int yi = ryi0;

	// Pre-steps
	edge += dedge * -GSVector4(step_x ? fx0 : fy0);
	D += -static_cast<int>(dD * (step_x ? fx0 : fy0)) * (step_x ? dxi : dyi);
	if (D >= (pos_D ? 0 : scaleD))
	{
		D -= scaleD;
		xi += step_x ? 0 : 1;
		yi += step_x ? 1 : 0;
	}
	else if (D < (pos_D ? -scaleD : 0))
	{
		D += scaleD;
		xi += step_x ? 0 : -1;
		yi += step_x ? -1 : 0;
	}

	pxAssert(pos_D ? (-scaleD <= D && D < 0) : (0 <= D && D < scaleD));

	while (true)
	{

		bool draw = m_scissor.left <= xi && xi < m_scissor.right &&
		            m_scissor.top <= yi && yi < m_scissor.bottom &&
		            IsOneOfMyScanlines(yi);

		const bool first = step_x ? (xi == rxi0) : (yi == ryi0);
		const bool last = step_x ? (xi == rxi1) : (yi == ryi1);

		if (first)
			draw = draw && draw_first;
		if (last)
			draw = draw && draw_last;

		if (draw)
		{
			if (has_edge)
			{
				const float d = (static_cast<float>(D) / scaleDf) + (pos_D ? 0.5f : -0.5f);
				if (d >= 0.0f)
				{
					const int cov = std::clamp(static_cast<int>(0x10000 * d), 0, 0xffff);
					
					AddScanline(e, 1, xi, yi, edge);
					
					e->p.U32[0] = 0xffff - cov;
					
					e++;

					AddScanline(e, 1, xi + (step_x ? 0 : 1), yi + (step_x ? 1 : 0), edge);

					e->p.U32[0] = cov;

					e++;
				}
				else if (d < 0.0f)
				{
					const int cov = static_cast<int>(0x10000 * (-d));

					AddScanline(e, 1, xi, yi, edge);

					e->p.U32[0] = 0xffff - cov;

					e++;

					AddScanline(e, 1, xi + (step_x ? 0 : -1), yi + (step_x ? -1 : 0), edge);

					e->p.U32[0] = cov;

					e++;
				}
			}
			else // No antialiasing.
			{
				AddScanline(e, 1, xi, yi, edge);

				e++;
			}

		}

		if (last)
			break;

		edge += dedge;
		D += dD;
		xi += step_x ? dxi : 0;
		yi += step_x ? 0 : dyi;
		if constexpr (pos_D)
		{
			if (D >= 0.0f)
			{
				D -= scaleD;
				xi += step_x ? 0 : 1;
				yi += step_x ? 1 : 0;
			}
		}
		else
		{
			if (D < 0.0f)
			{
				D += scaleD;
				xi += step_x ? 0 : -1;
				yi += step_x ? -1 : 0;
			}
		}
	}

	m_edge.count = e - m_edge.buff;
}

void GSRasterizer::DrawLine(const GSVertexSW* vertex, const u16* index)
{
	m_primcount++;

	const GSVertexSW& v0 = vertex[index[0]];
	const GSVertexSW& v1 = vertex[index[1]];

	GSVertexSW dv = v1 - v0;

	GSVector4 dp = dv.p.abs();

	int i = (dp < dp.yxwz()).mask() & 1; // |dx| <= |dy|

	if (HasEdge())
	{
		if (!i)
		{
			if (dv.p.x >= 0)
			{
				if (dv.p.y >= 0)
					DrawLineImpl<true, true, true, 1>(v0, v1, dv);
				else
					DrawLineImpl<true, true, false, 1>(v0, v1, dv);
			}
			else
			{
				if (dv.p.y >= 0)
					DrawLineImpl<true, false, true, 1>(v0, v1, dv);
				else
					DrawLineImpl<true, false, false, 1>(v0, v1, dv);
			}
		}
		else
		{
			if (dv.p.x >= 0)
			{
				if (dv.p.y >= 0)
					DrawLineImpl<false, true, true, 1>(v0, v1, dv);
				else
					DrawLineImpl<false, true, false, 1>(v0, v1, dv);
			}
			else
			{
				if (dv.p.y >= 0)
					DrawLineImpl<false, false, true, 1>(v0, v1, dv);
				else
					DrawLineImpl<false, false, false, 1>(v0, v1, dv);
			}
		}
	}
	else
	{
		if (!i)
		{
			if (dv.p.x >= 0)
			{
				if (dv.p.y >= 0)
					DrawLineImpl<true, true, true, 0>(v0, v1, dv);
				else
					DrawLineImpl<true, true, false, 0>(v0, v1, dv);
			}
			else
			{
				if (dv.p.y >= 0)
					DrawLineImpl<true, false, true, 0>(v0, v1, dv);
				else
					DrawLineImpl<true, false, false, 0>(v0, v1, dv);
			}
		}
		else
		{
			if (dv.p.x >= 0)
			{
				if (dv.p.y >= 0)
					DrawLineImpl<false, true, true, 0>(v0, v1, dv);
				else
					DrawLineImpl<false, true, false, 0>(v0, v1, dv);
			}
			else
			{
				if (dv.p.y >= 0)
					DrawLineImpl<false, false, true, 0>(v0, v1, dv);
				else
					DrawLineImpl<false, false, false, 0>(v0, v1, dv);
			}
		}
	}

	Flush(vertex, index, GSVertexSW::zero(), HasEdge());

	return;

	if (HasEdge())
	{
		DrawEdge(v0, v1, dv, i, 0);
		DrawEdge(v0, v1, dv, i, 1);

		Flush(vertex, index, GSVertexSW::zero(), true);

		return;
	}

	GSVector4i dpi(dp);

	if (dpi.y == 0)
	{
		if (dpi.x > 0)
		{
			// shortcut for horizontal lines

			GSVector4 mask = (v0.p > v1.p).xxxx();

			GSVertexSW scan;

			scan.p = v0.p.blend32(v1.p, mask);
			scan.t = v0.t.blend32(v1.t, mask);
			scan.c = v0.c.blend32(v1.c, mask);

			GSVector4i p(scan.p);

			if (m_scissor.top <= p.y && p.y < m_scissor.bottom && IsOneOfMyScanlines(p.y))
			{
				GSVector4 lrf = scan.p.upl(v1.p.blend32(v0.p, mask)).ceil();
				GSVector4 l = lrf.max(m_fscissor_x);
				GSVector4 r = lrf.min(m_fscissor_x);
				GSVector4i lr = GSVector4i(l.xxyy(r));

				int left = lr.extract32<0>();
				int right = lr.extract32<2>();

				int pixels = right - left;

				if (pixels > 0)
				{
					GSVertexSW dscan = dv / dv.p.xxxx();

					scan += dscan * (l - scan.p).xxxx();

					m_setup_prim(vertex, index, dscan, m_local);

					DrawScanline(pixels, left, p.y, scan);
				}
			}
		}

		return;
	}

	int steps = dpi.v[i];

	if (steps > 0)
	{
		GSVertexSW edge = v0;
		GSVertexSW dedge = dv / GSVector4(dp.v[i]);

		GSVertexSW* RESTRICT e = m_edge.buff;

		while (1)
		{
			GSVector4i p(edge.p);

			if (m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom)
			{
				if (IsOneOfMyScanlines(p.y))
				{
					AddScanline(e, 1, p.x, p.y, edge);

					e++;
				}
			}

			if (--steps == 0)
				break;

			edge += dedge;
		}

		m_edge.count = e - m_edge.buff;

		Flush(vertex, index, GSVertexSW::zero());
	}
}

static const u8 s_ysort[8][4] =
{
	{0, 1, 2, 0}, // y0 <= y1 <= y2
	{1, 0, 2, 0}, // y1 < y0 <= y2
	{0, 0, 0, 0},
	{1, 2, 0, 0}, // y1 <= y2 < y0
	{0, 2, 1, 0}, // y0 <= y2 < y1
	{0, 0, 0, 0},
	{2, 0, 1, 0}, // y2 < y0 <= y1
	{2, 1, 0, 0}, // y2 < y1 < y0
};

#if _M_SSE >= 0x501

void GSRasterizer::DrawTriangle(const GSVertexSW* vertex, const u16* index)
{
	m_primcount++;

	GSVertexSW2 edge;
	GSVertexSW2 dedge;
	GSVertexSW2 dscan;

	GSVector4 y0011 = vertex[index[0]].p.yyyy(vertex[index[1]].p);
	GSVector4 y1221 = vertex[index[1]].p.yyyy(vertex[index[2]].p).xzzx();

	int m1 = (y0011 > y1221).mask() & 7;

	int i[3];

	i[0] = index[s_ysort[m1][0]];
	i[1] = index[s_ysort[m1][1]];
	i[2] = index[s_ysort[m1][2]];

	const GSVertexSW2* _v = (const GSVertexSW2*)vertex;

	const GSVertexSW2& v0 = _v[i[0]];
	const GSVertexSW2& v1 = _v[i[1]];
	const GSVertexSW2& v2 = _v[i[2]];

	y0011 = v0.p.yyyy(v1.p);
	y1221 = v1.p.yyyy(v2.p).xzzx();

	m1 = (y0011 == y1221).mask() & 7;

	// if (i == 0) => y0 < y1 < y2
	// if (i == 1) => y0 == y1 < y2
	// if (i == 4) => y0 < y1 == y2

	if (m1 == 7) // y0 == y1 == y2
		return;

	GSVector4 tbf = y0011.xzxz(y1221).ceil();
	GSVector4 tbmax = tbf.max(m_fscissor_y);
	GSVector4 tbmin = tbf.min(m_fscissor_y);
	GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin)); // max(y0, t) max(y1, t) min(y1, b) min(y2, b)

	GSVertexSW2 dv0 = v1 - v0;
	GSVertexSW2 dv1 = v2 - v0;
	GSVertexSW2 dv2 = v2 - v1;

	GSVector4 cross = GSVector4::loadl(&dv0.p) * GSVector4::loadl(&dv1.p).yxwz();

	cross = (cross - cross.yxwz()).yyyy(); // select the second component, the negated cross product
	// the longest horizontal span would be cross.x / dv1.p.y, but we don't need its actual value

	int m2 = cross.upl(cross == GSVector4::zero()).mask();

	if (m2 & 2)
		return;

	m2 &= 1;

	GSVector4 dxy01 = dv0.p.xyxy(dv1.p);

	GSVector4 dx = dxy01.xzxy(dv2.p);
	GSVector4 dy = dxy01.ywyx(dv2.p);

	GSVector4 ddx[3];

	ddx[0] = dx / dy;
	ddx[1] = ddx[0].yxzw();
	ddx[2] = ddx[0].xzyw();

	// Precision is important here. Don't use reciprocal, it will break Jak3/Xenosaga1
	GSVector8 dxy01c(dxy01 / cross);

	dscan = dv1 * dxy01c.yyyy() - dv0 * dxy01c.wwww();
	dedge = dv0 * dxy01c.zzzz() - dv1 * dxy01c.xxxx();

	if (m1 & 1)
	{
		if (tb.y < tb.w)
		{
			edge = _v[i[1 - m2]];

			edge.p.y = vertex[i[m2]].p.x;
			dedge.p = ddx[!m2 << 1].yzzw(dedge.p);

			DrawTriangleSection(tb.x, tb.w, edge, dedge, dscan, vertex[i[1 - m2]].p);
		}
	}
	else
	{
		if (tb.x < tb.z)
		{
			edge = v0;

			edge.p.y = edge.p.x;
			dedge.p = ddx[m2].xyzw(dedge.p);

			DrawTriangleSection(tb.x, tb.z, edge, dedge, dscan, v0.p);
		}

		if (tb.y < tb.w)
		{
			edge = v1;

			edge.p = (v0.p.xxxx() + ddx[m2] * dv0.p.yyyy()).xyzw(edge.p);
			dedge.p = ddx[!m2 << 1].yzzw(dedge.p);

			DrawTriangleSection(tb.y, tb.w, edge, dedge, dscan, v1.p);
		}
	}

	Flush(vertex, index, (GSVertexSW&)dscan);

	if (HasEdge())
	{
		GSVector4 a = dx.abs() < dy.abs();       // |dx| <= |dy|
		GSVector4 b = dx < GSVector4::zero();    // dx < 0
		GSVector4 c = cross < GSVector4::zero(); // longest.p.x < 0

		int orientation = a.mask();
		int side = ((a | b) ^ c).mask() ^ 2; // evil

		DrawEdge((GSVertexSW&)v0, (GSVertexSW&)v1, (GSVertexSW&)dv0, orientation & 1, side & 1);
		DrawEdge((GSVertexSW&)v0, (GSVertexSW&)v2, (GSVertexSW&)dv1, orientation & 2, side & 2);
		DrawEdge((GSVertexSW&)v1, (GSVertexSW&)v2, (GSVertexSW&)dv2, orientation & 4, side & 4);

		Flush(vertex, index, GSVertexSW::zero(), true);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW2& RESTRICT edge, const GSVertexSW2& RESTRICT dedge, const GSVertexSW2& RESTRICT dscan, const GSVector4& RESTRICT p0)
{
	pxAssert(top < bottom);
	pxAssert(edge.p.x <= edge.p.y);

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];

	GSVector4 scissor = m_fscissor_x;

	top = FindMyNextScanline(top);

	while (top < bottom)
	{
		const float dy = static_cast<float>(top) - p0.y;
		GSVector8 dyv(dy);

		GSVector4 xy = GSVector4::loadl(&edge.p) + GSVector4::loadl(&dedge.p) * dyv.extract<0>();

		GSVector4 lrf = xy.ceil();
		GSVector4 l = lrf.max(scissor);
		GSVector4 r = lrf.min(scissor);
		GSVector4i lr = GSVector4i(l.xxyy(r));

		int left = lr.extract32<0>();
		int right = lr.extract32<2>();

		int pixels = right - left;

		if (pixels > 0)
		{
			float prestep = l.x - p0.x;
			GSVector8 prestepv(prestep);

			reinterpret_cast<GSVertexSW2*>(e)->p.F64[1] = edge.p.F64[1] + dedge.p.F64[1] * dy + dscan.p.F64[1] * prestep;
			reinterpret_cast<GSVertexSW2*>(e)->tc = edge.tc + dedge.tc * dyv + dscan.tc * prestepv;

			AddScanlineInfo(e++, pixels, left, top);
		}

		top++;

		if (!IsOneOfMyScanlines(top))
		{
			top += (m_threads - 1) << m_thread_height;
		}
	}

	m_edge.count += e - &m_edge.buff[m_edge.count];
}

#else

void GSRasterizer::DrawTriangle(const GSVertexSW* vertex, const u16* index)
{
	m_primcount++;

	GSVertexSW edge;
	GSVertexSW dedge;
	GSVertexSW dscan;

	GSVector4 y0011 = vertex[index[0]].p.yyyy(vertex[index[1]].p);
	GSVector4 y1221 = vertex[index[1]].p.yyyy(vertex[index[2]].p).xzzx();

	int m1 = (y0011 > y1221).mask() & 7;

	int i[3];

	i[0] = index[s_ysort[m1][0]];
	i[1] = index[s_ysort[m1][1]];
	i[2] = index[s_ysort[m1][2]];

	const GSVertexSW& v0 = vertex[i[0]];
	const GSVertexSW& v1 = vertex[i[1]];
	const GSVertexSW& v2 = vertex[i[2]];

	y0011 = v0.p.yyyy(v1.p);
	y1221 = v1.p.yyyy(v2.p).xzzx();

	m1 = (y0011 == y1221).mask() & 7;

	// if (i == 0) => y0 < y1 < y2
	// if (i == 1) => y0 == y1 < y2
	// if (i == 4) => y0 < y1 == y2

	if (m1 == 7)
		return; // y0 == y1 == y2

	GSVector4 tbf = y0011.xzxz(y1221).ceil();
	GSVector4 tbmax = tbf.max(m_fscissor_y);
	GSVector4 tbmin = tbf.min(m_fscissor_y);
	GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin)); // max(y0, t) max(y1, t) min(y1, b) min(y2, b)

	GSVertexSW dv0 = v1 - v0;
	GSVertexSW dv1 = v2 - v0;
	GSVertexSW dv2 = v2 - v1;

	GSVector4 cross = GSVector4::loadl(&dv0.p) * GSVector4::loadl(&dv1.p).yxwz();

	cross = (cross - cross.yxwz()).yyyy(); // select the second component, the negated cross product
	// the longest horizontal span would be cross.x / dv1.p.y, but we don't need its actual value

	int m2 = cross.upl(cross == GSVector4::zero()).mask();

	if (m2 & 2)
		return;

	m2 &= 1;

	GSVector4 dxy01 = dv0.p.xyxy(dv1.p);

	GSVector4 dx = dxy01.xzxy(dv2.p);
	GSVector4 dy = dxy01.ywyx(dv2.p);

	GSVector4 ddx[3];

	ddx[0] = dx / dy;
	ddx[1] = ddx[0].yxzw();
	ddx[2] = ddx[0].xzyw();

	// Precision is important here. Don't use reciprocal, it will break Jak3/Xenosaga1
	GSVector4 dxy01c = dxy01 / cross;

	dscan = dv1 * dxy01c.yyyy() - dv0 * dxy01c.wwww();
	dedge = dv0 * dxy01c.zzzz() - dv1 * dxy01c.xxxx();

	if (m1 & 1)
	{
		if (tb.y < tb.w)
		{
			edge = vertex[i[1 - m2]];

			edge.p.y = vertex[i[m2]].p.x;
			dedge.p = ddx[!m2 << 1].yzzw(dedge.p);

			DrawTriangleSection(tb.x, tb.w, edge, dedge, dscan, vertex[i[1 - m2]].p);
		}
	}
	else
	{
		if (tb.x < tb.z)
		{
			edge = v0;

			edge.p.y = edge.p.x;
			dedge.p = ddx[m2].xyzw(dedge.p);

			DrawTriangleSection(tb.x, tb.z, edge, dedge, dscan, v0.p);
		}

		if (tb.y < tb.w)
		{
			edge = v1;

			edge.p = (v0.p.xxxx() + ddx[m2] * dv0.p.yyyy()).xyzw(edge.p);
			dedge.p = ddx[!m2 << 1].yzzw(dedge.p);

			DrawTriangleSection(tb.y, tb.w, edge, dedge, dscan, v1.p);
		}
	}

	Flush(vertex, index, dscan);

	if (HasEdge())
	{
		GSVector4 a = dx.abs() < dy.abs();       // |dx| <= |dy|
		GSVector4 b = dx < GSVector4::zero();    // dx < 0
		GSVector4 c = cross < GSVector4::zero(); // longest.p.x < 0

		int orientation = a.mask();
		int side = ((a | b) ^ c).mask() ^ 2; // evil

		DrawEdge(v0, v1, dv0, orientation & 1, side & 1);
		DrawEdge(v0, v2, dv1, orientation & 2, side & 2);
		DrawEdge(v1, v2, dv2, orientation & 4, side & 4);

		Flush(vertex, index, GSVertexSW::zero(), true);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& RESTRICT edge, const GSVertexSW& RESTRICT dedge, const GSVertexSW& RESTRICT dscan, const GSVector4& RESTRICT p0)
{
	pxAssert(top < bottom);
	pxAssert(edge.p.x <= edge.p.y);

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];

	GSVector4 scissor = m_fscissor_x;

	top = FindMyNextScanline(top);

	while (top < bottom)
	{
		const float dy = static_cast<float>(top) - p0.y;

		GSVector4 xy = GSVector4::loadl(&edge.p) + GSVector4::loadl(&dedge.p) * dy;

		GSVector4 lrf = xy.ceil();
		GSVector4 l = lrf.max(scissor);
		GSVector4 r = lrf.min(scissor);
		GSVector4i lr = GSVector4i(l.xxyy(r));

		int left = lr.extract32<0>();
		int right = lr.extract32<2>();

		int pixels = right - left;

		if (pixels > 0)
		{
			const float prestep = l.x - p0.x;

			e->p.F64[1] = edge.p.F64[1] + dedge.p.F64[1] * dy + dscan.p.F64[1] * prestep;
			e->t = edge.t + dedge.t * dy + dscan.t * prestep;
			e->c = edge.c + dedge.c * dy + dscan.c * prestep;

			AddScanlineInfo(e++, pixels, left, top);
		}

		top++;

		if (!IsOneOfMyScanlines(top))
		{
			top += (m_threads - 1) << m_thread_height;
		}
	}

	m_edge.count += e - &m_edge.buff[m_edge.count];
}

#endif

void GSRasterizer::DrawSprite(const GSVertexSW* vertex, const u16* index)
{
	m_primcount++;

	const GSVertexSW& v0 = vertex[index[0]];
	const GSVertexSW& v1 = vertex[index[1]];

	GSVector4 mask = (v0.p < v1.p).xyzw(GSVector4::zero());

	GSVertexSW v[2];

	v[0].p = v1.p.blend32(v0.p, mask);
	v[0].t = v1.t.blend32(v0.t, mask);
	v[0].c = v1.c;

	v[1].p = v0.p.blend32(v1.p, mask);
	v[1].t = v0.t.blend32(v1.t, mask);

	GSVector4i r(v[0].p.xyxy(v[1].p).ceil());

	r = r.rintersect(m_scissor);

	if (r.rempty())
		return;

	GSVertexSW scan = v[0];

	if ((m_scanmsk_value & 2) == 0 && m_local.gd->sel.IsSolidRect())
	{
		if (m_threads == 1)
		{
			GSDrawScanline::DrawRect(r, scan, m_local);

			int pixels = r.width() * r.height();

			m_pixels.actual += pixels;
			m_pixels.total += pixels;
		}
		else
		{
			int top = FindMyNextScanline(r.top);
			int bottom = r.bottom;

			while (top < bottom)
			{
				r.top = top;
				r.bottom = std::min<int>((top + (1 << m_thread_height)) & ~((1 << m_thread_height) - 1), bottom);

				GSDrawScanline::DrawRect(r, scan, m_local);

				int pixels = r.width() * r.height();

				m_pixels.actual += pixels;
				m_pixels.total += pixels;

				top = r.bottom + ((m_threads - 1) << m_thread_height);
			}
		}

		return;
	}

	GSVector4 dxy = v[1].p - v[0].p;
	GSVector4 duv = v[1].t - v[0].t;

	GSVector4 dt = duv / dxy;

	GSVertexSW dedge;
	GSVertexSW dscan;

	dedge.t = GSVector4::zero().insert32<1, 1>(dt);
	dscan.t = GSVector4::zero().insert32<0, 0>(dt);

	GSVector4 prestep = GSVector4(r.left, r.top) - scan.p;

	scan.t = (scan.t + dt * prestep).xyzw(scan.t);

	m_setup_prim(vertex, index, dscan, m_local);

	while (1)
	{
		if (IsOneOfMyScanlines(r.top))
		{
			DrawScanline(r.width(), r.left, r.top, scan);
		}

		if (++r.top >= r.bottom)
			break;

		scan.t += dedge.t;
	}
}

void GSRasterizer::DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, int orientation, int side)
{
	// orientation:
	// - true: |dv.p.y| > |dv.p.x|
	// - false |dv.p.x| > |dv.p.y|
	// side:
	// - true: top/left edge
	// - false: bottom/right edge

	// TODO: bit slow and too much duplicated code
	// TODO: inner pre-step is still missing (hardly noticable)
	// TODO: it does not always line up with the edge of the surrounded triangle

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];

	if (orientation)
	{
		GSVector4 tbf = v0.p.yyyy(v1.p).ceil(); // t t b b
		GSVector4 tbmax = tbf.max(m_fscissor_y); // max(t, st) max(t, sb) max(b, st) max(b, sb)
		GSVector4 tbmin = tbf.min(m_fscissor_y); // min(t, st) min(t, sb) min(b, st) min(b, sb)
		GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin)); // max(t, st) max(b, sb) min(t, st) min(b, sb)

		int top, bottom;

		GSVertexSW edge, dedge;

		if (dv.p.y >= 0)
		{
			top    = tb.extract32<0>(); // max(t, st)
			bottom = tb.extract32<3>(); // min(b, sb)

			if (top >= bottom)
				return;

			edge = v0;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.xxxx() - edge.p.yyyy());
		}
		else
		{
			top    = tb.extract32<1>(); // max(b, st)
			bottom = tb.extract32<2>(); // min(t, sb)

			if (top >= bottom)
				return;

			edge = v1;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.zzzz() - edge.p.yyyy());
		}

		GSVector4i p = GSVector4i(edge.p.upl(dedge.p) * 0x10000);

		int x = p.extract32<0>();
		int dx = p.extract32<1>();

		if (side)
		{
			while (1)
			{
				int xi = x >> 16;
				int xf = x & 0xffff;

				if (m_scissor.left <= xi && xi < m_scissor.right && IsOneOfMyScanlines(top))
				{
					AddScanline(e, 1, xi, top, edge);

					e->p.U32[0] = (0x10000 - xf) & 0xffff;

					e++;
				}

				if (++top >= bottom)
					break;

				edge += dedge;
				x += dx;
			}
		}
		else
		{
			while (1)
			{
				int xi = (x >> 16) + 1;
				int xf = x & 0xffff;

				if (m_scissor.left <= xi && xi < m_scissor.right && IsOneOfMyScanlines(top))
				{
					AddScanline(e, 1, xi, top, edge);

					e->p.U32[0] = xf;

					e++;
				}

				if (++top >= bottom)
					break;

				edge += dedge;
				x += dx;
			}
		}
	}
	else
	{
		GSVector4 lrf = v0.p.xxxx(v1.p).ceil(); // l l r r
		GSVector4 lrmax = lrf.max(m_fscissor_x); // max(l, sl) max(l, sr) max(r, sl) max(r, sr)
		GSVector4 lrmin = lrf.min(m_fscissor_x); // min(l, sl) min(l, sr) min(r, sl) min(r, sr)
		GSVector4i lr = GSVector4i(lrmax.xzyw(lrmin)); // max(l, sl) max(r, sl) min(l, sr) min(r, sr)

		int left, right;

		GSVertexSW edge, dedge;

		if ((dv.p >= GSVector4::zero()).mask() & 1)
		{
			left  = lr.extract32<0>(); // max(l, sl)
			right = lr.extract32<3>(); // min(r, sr)

			if (left >= right)
				return;

			edge = v0;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.xxxx() - edge.p.xxxx());
		}
		else
		{
			left  = lr.extract32<1>(); // max(r, sl)
			right = lr.extract32<2>(); // min(l, sr)

			if (left >= right)
				return;

			edge = v1;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.zzzz() - edge.p.xxxx());
		}

		GSVector4i p = GSVector4i(edge.p.upl(dedge.p) * 0x10000);

		int y = p.extract32<2>();
		int dy = p.extract32<3>();

		if (side)
		{
			while (1)
			{
				int yi = y >> 16;
				int yf = y & 0xffff;

				if (m_scissor.top <= yi && yi < m_scissor.bottom && IsOneOfMyScanlines(yi))
				{
					AddScanline(e, 1, left, yi, edge);

					e->p.U32[0] = (0x10000 - yf) & 0xffff;

					e++;
				}

				if (++left >= right)
					break;

				edge += dedge;
				y += dy;
			}
		}
		else
		{
			while (1)
			{
				int yi = (y >> 16) + 1;
				int yf = y & 0xffff;

				if (m_scissor.top <= yi && yi < m_scissor.bottom && IsOneOfMyScanlines(yi))
				{
					AddScanline(e, 1, left, yi, edge);

					e->p.U32[0] = yf;

					e++;
				}

				if (++left >= right)
					break;

				edge += dedge;
				y += dy;
			}
		}
	}

	m_edge.count += e - &m_edge.buff[m_edge.count];
}

void GSRasterizer::AddScanline(GSVertexSW* e, int pixels, int left, int top, const GSVertexSW& scan)
{
	*e = scan;
	AddScanlineInfo(e, pixels, left, top);
}

void GSRasterizer::Flush(const GSVertexSW* vertex, const u16* index, const GSVertexSW& dscan, bool edge /* = false */)
{
	// TODO: on win64 this could be the place where xmm6-15 are preserved (not by each DrawScanline)

	int count = m_edge.count;

	if (count > 0)
	{
		m_setup_prim(vertex, index, dscan, m_local);

		const GSVertexSW* RESTRICT e = m_edge.buff;
		const GSVertexSW* RESTRICT ee = e + count;

		if (!edge)
		{
			do
			{
				int pixels = e->_pad.I32[0];
				int left = e->_pad.I32[1];
				int top = e->_pad.I32[2];

				DrawScanline(pixels, left, top, *e++);
			} while (e < ee);
		}
		else
		{
			do
			{
				int pixels = e->_pad.I32[0];
				int left = e->_pad.I32[1];
				int top = e->_pad.I32[2];

				DrawEdge(pixels, left, top, *e++);
			} while (e < ee);
		}

		m_edge.count = 0;
	}
}

#if _M_SSE >= 0x501
#define PIXELS_PER_LOOP 8
#else
#define PIXELS_PER_LOOP 4
#endif

void GSRasterizer::DrawScanline(int pixels, int left, int top, const GSVertexSW& scan)
{
	if ((m_scanmsk_value & 2) && (m_scanmsk_value & 1) == (top & 1)) return;
	m_pixels.actual += pixels;
	m_pixels.total += ((left + pixels + (PIXELS_PER_LOOP - 1)) & ~(PIXELS_PER_LOOP - 1)) - (left & ~(PIXELS_PER_LOOP - 1));
	//m_pixels.total += ((left + pixels + (PIXELS_PER_LOOP - 1)) & ~(PIXELS_PER_LOOP - 1)) - left;

	pxAssert(m_pixels.actual <= m_pixels.total);

	m_draw_scanline(pixels, left, top, scan, m_local);
}

void GSRasterizer::DrawEdge(int pixels, int left, int top, const GSVertexSW& scan)
{
	if ((m_scanmsk_value & 2) && (m_scanmsk_value & 1) == (top & 1)) return;
	m_pixels.actual += 1;
	m_pixels.total += PIXELS_PER_LOOP - 1;

	pxAssert(m_pixels.actual <= m_pixels.total);

	m_draw_edge(pixels, left, top, scan, m_local);
}

//

GSSingleRasterizer::GSSingleRasterizer()
	: m_r(&m_ds, 0, 1)
{
}

GSSingleRasterizer::~GSSingleRasterizer() = default;

void GSSingleRasterizer::Queue(const GSRingHeap::SharedPtr<GSRasterizerData>& data)
{
	Draw(*data.get());
}

void GSSingleRasterizer::Draw(GSRasterizerData& data)
{
	if (!m_ds.SetupDraw(data)) [[unlikely]]
	{
		m_ds.ResetCodeCache();
		m_ds.SetupDraw(data);
	}

	m_r.Draw(data);
}

void GSSingleRasterizer::Sync()
{
}

bool GSSingleRasterizer::IsSynced() const
{
	return true;
}

int GSSingleRasterizer::GetPixels(bool reset /*= true*/)
{
	return m_r.GetPixels(reset);
}

void GSSingleRasterizer::PrintStats()
{
#ifdef ENABLE_DRAW_STATS
	m_ds.PrintStats();
#endif
}

//

GSRasterizerList::GSRasterizerList(int threads)
{
	m_thread_height = compute_best_thread_height(threads);

	const int rows = (2048 >> m_thread_height) + 16;
	m_scanline = static_cast<u8*>(_aligned_malloc(rows, 64));

	for (int i = 0; i < rows; i++)
	{
		m_scanline[i] = static_cast<u8>(i % threads);
	}

	PerformanceMetrics::SetGSSWThreadCount(threads);
}

GSRasterizerList::~GSRasterizerList()
{
	PerformanceMetrics::SetGSSWThreadCount(0);
	_aligned_free(m_scanline);
}

void GSRasterizerList::OnWorkerStartup(int i, u64 affinity)
{
	Threading::SetNameOfCurrentThread(StringUtil::StdStringFromFormat("GS-SW-%d", i).c_str());

	Threading::ThreadHandle handle(Threading::ThreadHandle::GetForCallingThread());
	if (affinity != 0)
	{
		INFO_LOG("Pinning GS thread {} to CPU {} (0x{:x})", i, std::countr_zero(affinity), affinity);
		handle.SetAffinity(affinity);
	}

	PerformanceMetrics::SetGSSWThread(i, std::move(handle));
}

void GSRasterizerList::OnWorkerShutdown(int i)
{
}

void GSRasterizerList::Queue(const GSRingHeap::SharedPtr<GSRasterizerData>& data)
{
	GSVector4i r = data->bbox.rintersect(data->scissor);

	if (!m_ds.SetupDraw(*data.get())) [[unlikely]]
	{
		Sync();
		m_ds.ResetCodeCache();
		m_ds.SetupDraw(*data.get());
	}

	pxAssert(r.top >= 0 && r.top < 2048 && r.bottom >= 0 && r.bottom < 2048);

	int top = r.top >> m_thread_height;
	int bottom = std::min<int>((r.bottom + (1 << m_thread_height) - 1) >> m_thread_height, top + m_workers.size());

	while (top < bottom)
	{
		m_workers[m_scanline[top++]]->Push(data);
	}
}

void GSRasterizerList::Sync()
{
	if (!IsSynced())
	{
		for (size_t i = 0; i < m_workers.size(); i++)
		{
			m_workers[i]->Wait();
		}

		g_perfmon.Put(GSPerfMon::SyncPoint, 1);
	}
}

bool GSRasterizerList::IsSynced() const
{
	for (size_t i = 0; i < m_workers.size(); i++)
	{
		if (!m_workers[i]->IsEmpty())
		{
			return false;
		}
	}

	return true;
}

int GSRasterizerList::GetPixels(bool reset)
{
	int pixels = 0;

	for (size_t i = 0; i < m_workers.size(); i++)
	{
		pixels += m_r[i]->GetPixels(reset);
	}

	return pixels;
}

std::unique_ptr<IRasterizer> GSRasterizerList::Create(int threads)
{
	threads = std::max<int>(threads, 0);

	if (threads == 0)
	{
		return std::make_unique<GSSingleRasterizer>();
	}

	std::unique_ptr<GSRasterizerList> rl(new GSRasterizerList(threads));

	const std::vector<u32>& procs = VMManager::Internal::GetSoftwareRendererProcessorList();
	const bool pin = (EmuConfig.EnableThreadPinning && static_cast<size_t>(threads) <= procs.size());
	if (EmuConfig.EnableThreadPinning && !pin)
		WARNING_LOG("Not pinning SW threads, we need {} processors, but only have {}", threads, procs.size());

	for (int i = 0; i < threads; i++)
	{
		const u64 affinity = pin ? (static_cast<u64>(1u) << procs[i]) : 0;
		rl->m_r.push_back(std::unique_ptr<GSRasterizer>(new GSRasterizer(&rl->m_ds, i, threads)));
		auto& r = *rl->m_r[i];
		rl->m_workers.push_back(std::unique_ptr<GSWorker>(new GSWorker(
			[i, affinity]() { GSRasterizerList::OnWorkerStartup(i, affinity); },
			[&r](GSRingHeap::SharedPtr<GSRasterizerData>& item) { r.Draw(*item.get()); },
			[i]() { GSRasterizerList::OnWorkerShutdown(i); })));
	}

	return rl;
}

void GSRasterizerList::PrintStats()
{
}

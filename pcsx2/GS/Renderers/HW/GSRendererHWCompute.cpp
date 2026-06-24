#include "GS/Renderers/HW/GSRendererHWCompute.h"
#include "GS/GSGL.h"
#include "GS/GSPerfMon.h"
#include "GS/GSUtil.h"
#include "Host.h"
#include "common/Console.h"
#include "common/BitUtils.h"
#include "common/StringUtil.h"
#include <bit>
#include <memory>


GSRendererHWCompute::GSRendererHWCompute()
	: m_texture_cache(std::make_unique<GSTextureCacheCompute>())
{
}

GSRendererHWCompute::~GSRendererHWCompute()
{
}

GSTexture* GSRendererHWCompute::GetOutput(int i, float& scale, int& y_offset)
{
	return nullptr;
}

void GSRendererHWCompute::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
}

void GSRendererHWCompute::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut)
{
}

void GSRendererHWCompute::ConvertVertices(GSVertex* input, u32 count, std::vector<GSHybridVertex>& output)
{
	const float offset_x = static_cast<float>(m_context->XYOFFSET.OFX);
	const float offset_y = static_cast<float>(m_context->XYOFFSET.OFY);

	output.resize(count);
	for (u32 i = 0; i < count; i++)
	{
		const GSVertex& vin = input[i];
		GSHybridVertex& vout = output[i];

		vout.p.x = (static_cast<float>(vin.XYZ.X) - offset_x) / 16.0f;
		vout.p.y = (static_cast<float>(vin.XYZ.Y) - offset_y) / 16.0f;

		vout.z = vin.XYZ.Z;

		if (PRIM->TME)
		{
			if (PRIM->FST)
			{
				vout.t.x = static_cast<float>(vin.U) / 16.0f;
				vout.t.y = static_cast<float>(vin.V) / 16.0f;
			}
			else
			{
				vout.t.x = static_cast<float>(vin.ST.S);
				vout.t.y = static_cast<float>(vin.ST.T);
				vout.q = vin.RGBAQ.Q;
			}
		}

		vout.c = vin.RGBAQ.U32[0];

		vout.f = vin.FOG;
	}
}

GSHybridDrawMesh GSRendererHWCompute::GetDrawMesh()
{
	GSHybridDrawMesh mesh;

	// Convert vertices for the hybrid pipeline.
	ConvertVertices(m_vertex->buff, m_vertex->tail, m_draw_vertices);
	
	mesh.vertices = m_draw_vertices.data();
	mesh.num_vertices = m_vertex->tail;
	mesh.indices = m_index->buff;
	mesh.num_indices = m_index->tail;

	return mesh;
}

GSRendererHWCompute::CachedDrawTextures GSRendererHWCompute::LookupCachedDrawTextures()
{
	const GIFRegFRAME& frame = m_context->FRAME;
	const GIFRegZBUF& zbuf = m_context->ZBUF;
	const GIFRegTEX0& tex0 = m_context->TEX0;
	const GIFRegTEXCLUT& texclut = m_env.TEXCLUT;

	CachedDrawTextures textures;

	// Sampled texture
	if (PRIM->TME)
	{
		GSMemoryRect rect(tex0);

		GSMemoryCLUT clut = GSMemoryCLUT::Create(tex0, texclut);

		textures.tex = m_texture_cache->LookupTexture(rect, clut);
	}

	// Frame
	{
		GSTextureCacheCompute::GSMemoryRect rect(frame, 512);

		textures.rt = m_texture_cache->LookupTexture(rect);
	}

	// Z buffer
	if (zbuf.ZMSK == 0)
	{
		GSTextureCacheCompute::GSMemoryRect rect(zbuf, frame, 512);

		textures.depth = m_texture_cache->LookupTexture(rect);
	}

	return textures;
}

GSHybridDrawTextures GSRendererHWCompute::GetBackendDrawTextures(const CachedDrawTextures& cached)
{
	GSHybridDrawTextures backend{};

	if (cached.rt)
		backend.rt = cached.rt->GetTexture();

	if (cached.depth)
		backend.depth = cached.depth->GetTexture();

	if (cached.tex)
	{
		backend.tex = cached.tex->GetTexture();
		backend.clut = cached.tex->GetCLUT();
	}

	return backend;
}

u32 GSRendererHWCompute::GetBackendTopology()
{
	switch (m_vt.m_primclass)
	{
		case GS_POINT_CLASS:
			return 0;
		case GS_LINE_CLASS:
			return 1;
		case GS_SPRITE_CLASS:
		case GS_TRIANGLE_CLASS:
			return 2;
		default:
			pxAssert(false);
			return 0xFFFFFFFFu;
	}
}

GSVector4i GSRendererHWCompute::GetViewport(const GSHybridDrawTextures& textures)
{
	if (textures.rt)
		return textures.rt->GetRect();
	else if (textures.depth)
		return textures.depth->GetRect();
	else
		return GSVector4i::zero();
}

GSHybridDrawSelector GSRendererHWCompute::GetSelector(const CachedDrawTextures& textures)
{
	GSHybridDrawSelector selector{};

	SetHybridSelectorBits(GS_HYBRID_SELECTOR_HAS_RT_BITS, textures.rt ? 1 : 0, selector);
	SetHybridSelectorBits(GS_HYBRID_SELECTOR_HAS_DEPTH_BITS, textures.depth ? 1 : 0, selector);
	if (textures.depth)
		SetHybridSelectorBits(GS_HYBRID_SELECTOR_ZTST_BITS, m_context->TEST.ZTST, selector);

	return selector;
}

GSHybridConstantBuffer GSRendererHWCompute::GetConstantBuffer(const CachedDrawTextures& textures)
{
	GSHybridConstantBuffer constants{};

	GSVector2i frame_size{};
	if (textures.rt)
		frame_size = textures.rt->GetSize();
	else if (textures.depth)
		frame_size = textures.depth->GetSize();
	
	constants.frame_size.x = static_cast<float>(frame_size.x);
	constants.frame_size.y = static_cast<float>(frame_size.y);

	return constants;
}

void GSRendererHWCompute::Draw()
{
	const GIFRegFRAME& frame = m_context->FRAME;
	const GIFRegTEX0& tex0 = m_context->TEX0;
	const GIFRegTEXCLUT& texclut = m_env.TEXCLUT;

	CachedDrawTextures cached_textures = LookupCachedDrawTextures();
	GSHybridDrawTextures backend_textures = GetBackendDrawTextures(cached_textures);

	GSHybridDrawMesh draw_mesh = GetDrawMesh();

	GSHybridDrawConfig config{};

	config.textures = backend_textures;
	config.mesh = draw_mesh;
	config.topology = GetBackendTopology();

	config.viewport = GetViewport(backend_textures);
	config.scissor = config.viewport; // FIXME

	config.selector = GetSelector(cached_textures);
	config.constants = GetConstantBuffer(cached_textures);

	g_gs_device->DrawHybrid(config);

	m_texture_cache->WritebackAllTextures();
}
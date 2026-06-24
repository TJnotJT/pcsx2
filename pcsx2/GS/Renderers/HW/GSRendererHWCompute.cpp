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

void GSRendererHWCompute::Draw()
{
	const GIFRegFRAME& frame = m_context->FRAME;
	const GIFRegTEX0& tex0 = m_context->TEX0;
	const GIFRegTEXCLUT& texclut = m_env.TEXCLUT;

	if (PRIM->TME)
	{
		GSTextureCacheCompute::GSMemoryRect rect;
		rect.BP = tex0.TBP0;
		rect.BW = tex0.TBW;
		rect.PSM = tex0.PSM;
		const int tw = 1 << tex0.TW;
		const int th = 1 << tex0.TH;
		rect.rect = GSVector4i(0, 0, tw, th);

		GSTextureCacheCompute::GSMemoryCLUT clut{};
		if (GSLocalMemory::m_psm[tex0.PSM].pal)
		{
			clut.CBP = tex0.CBP;
			clut.CBW = texclut.CBW;
			clut.CSM = tex0.CSM;
			clut.CPSM = tex0.CPSM;
			clut.COU = texclut.COU;
			clut.COV = texclut.COV;
			clut.PSM = tex0.PSM;
		}

		GSTextureCacheCompute::GSCachedTexture* tex = m_texture_cache->LookupTexture(rect, clut);
	}

	{
		GSTextureCacheCompute::GSMemoryRect rect;
		rect.BP = frame.Block();
		rect.BW = frame.FBW;
		rect.PSM = frame.PSM;
		rect.rect = GSVector4i(0, 0, frame.FBW * 64, 512);

		GSTextureCacheCompute::GSCachedTexture* rt = m_texture_cache->LookupTexture(rect);
	}

	m_texture_cache->WritebackAllTextures();
}
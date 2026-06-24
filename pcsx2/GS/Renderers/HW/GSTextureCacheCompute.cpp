#include "GS/Renderers/HW/GSTextureCacheCompute.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GS/GSUtil.h"
#include "common/SmallString.h"

#include <algorithm>
#include <memory>
#include <fmt/format.h>

GSTextureCacheCompute::GSTextureCacheCompute()
{
}

GSTextureCacheCompute::GSCachedTexture::~GSCachedTexture()
{
	if (m_texture)
	{
		g_gs_device->Recycle(m_texture.get());
		m_texture.release();
	}
	if (m_clut)
	{
		g_gs_device->Recycle(m_clut.get());
		m_clut.release();
	}
}

GSTextureCacheCompute::GSCachedTexture::GSCachedTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut)
	: m_texture{}
	, m_clut{}
	, m_mem_rect(rect)
	, m_mem_clut(clut)
	, m_state(TextureState::Clean)
{
	GSVector2i size(rect.rect.width(), rect.rect.height());
	const bool need_clut = GSLocalMemory::m_psm[rect.PSM].pal;

	// Create the texture.
	if (GSLocalMemory::m_psm[rect.PSM].depth)
		m_texture.reset(g_gs_device->CreateShaderWriteTarget(size, GSTexture::Format::DepthColor, false));
	else
		m_texture.reset(g_gs_device->CreateShaderWriteTarget(size, GSTexture::Format::Color, false));

	// Create a CLUT if the PSM requires one.
	if (need_clut)
	{
		m_clut.reset(g_gs_device->CreateShaderWriteTarget(GSVector2i(256, 1), GSTexture::Format::Color, false));
		g_gs_device->DoCLUT(m_clut.get(), rect.PSM, clut.CBP, clut.CBW, clut.CPSM, clut.CSM, clut.COU, clut.COV);

#if PCSX2_DEVBUILD
		m_texture->SetDebugName(fmt::format("CLUT {} @ 0x{:X} CBW={}",
			GSUtil::GetPSMName(clut.CPSM), clut.CBP, clut.CBW));
#endif
	}

	g_gs_device->DoSwizzle(MEM_TO_TEXTURE, m_texture.get(), m_clut.get(), rect.BP, rect.BW, rect.PSM,
		rect.GetX(), rect.GetY(), 0, 0, rect.GetWidth(), rect.GetHeight());

#if PCSX2_DEVBUILD
	if (need_clut)
	{
		m_texture->SetDebugName(fmt::format("{}x{} {} @ 0x{:X} TBW={} CBP=0x{:X}",
			rect.GetWidth(), rect.GetHeight(), GSUtil::GetPSMName(rect.PSM), rect.BP, rect.BW, clut.CBP));
	}
	else
	{
		m_texture->SetDebugName(fmt::format("{}x{} {} @ 0x{:X} TBW={}",
			rect.GetWidth(), rect.GetHeight(), GSUtil::GetPSMName(rect.PSM), rect.BP, rect.BW));
	}
#endif
}

void GSTextureCacheCompute::GSCachedTexture::WritebackIfDirty()
{
	if (IsDirty())
	{
		pxAssert(!HasCLUT()); // A texture with a CLUT should never be dirty.

		g_gs_device->DoSwizzle(TEXTURE_TO_MEM, m_texture.get(), nullptr, m_mem_rect.BP, m_mem_rect.BW, m_mem_rect.PSM,
			m_mem_rect.GetX(), m_mem_rect.GetY(), 0, 0, m_mem_rect.GetWidth(), m_mem_rect.GetHeight());

		m_state = TextureState::Clean;
	}
}

bool GSTextureCacheCompute::GSCachedTexture::Matches(const GSMemoryRect& rect, const GSMemoryCLUT& clut) const
{
	return (m_mem_rect == rect) && (!HasCLUT() || clut == m_mem_clut);
}

GSTextureCacheCompute::GSCachedTexture* GSTextureCacheCompute::LookupTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut)
{
	const auto it = std::find_if(m_cache.begin(), m_cache.end(),
		[&](const std::unique_ptr<GSCachedTexture>& tex) { return tex->Matches(rect, clut); });

	if (it == m_cache.end())
	{
		// No match, create a new cached texture.
		std::unique_ptr<GSCachedTexture> tex(new GSCachedTexture(rect, clut));
		m_cache.insert(m_cache.begin(), std::move(tex));
	}
	else
	{
		// Found a match, move it to the beginning.
		std::rotate(m_cache.begin(), it, it + 1);
	}

	return m_cache.begin()->get();
}

void GSTextureCacheCompute::WritebackAllTextures()
{
	for (std::unique_ptr<GSCachedTexture>& tex : m_cache)
		tex->WritebackIfDirty();
	m_cache.clear();
}
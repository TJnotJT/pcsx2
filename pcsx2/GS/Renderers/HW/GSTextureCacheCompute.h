// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/Renderers/Common/GSTexture.h"
#include "GS/Renderers/Common/GSRenderer.h"

#include <unordered_set>
#include <utility>
#include <limits>

class GSTextureCacheCompute
{
public:
	enum class TextureState
	{
		Clean,   // Synced with memory.
		Dirty,   // Needs to be written to memory.
		Invalid, // Needs to be read from memory.
	};

	struct GSMemoryRect
	{
		u32 BP;
		u32 BW;
		u32 PSM;
		GSVector4i rect;

		int GetX() const { return rect.x; }
		int GetY() const { return rect.y; }
		int GetWidth() const { return rect.width(); }
		int GetHeight() const { return rect.height(); }

		bool operator==(const GSMemoryRect& other) const
		{
			return BP == other.BP && BW == other.BW &&
				PSM == other.PSM && rect.eq(other.rect);
		}

		bool operator!=(const GSMemoryRect& other) const
		{
			return !(*this == other);
		}
	};

	struct GSMemoryCLUT
	{
		u32 PSM;
		u32 CBP;
		u32 CBW;
		u32 CPSM;
		u32 CSM;
		u32 COU;
		u32 COV;

		bool operator==(const GSMemoryCLUT& other) const
		{
			return PSM == other.PSM && CBP == other.CBP && CBW == other.CBW &&
				CPSM == other.CPSM && CSM == other.CSM && COU == other.COU &&
				COV == other.COV;
		}

		bool operator!=(const GSMemoryCLUT& other) const
		{
			return !(*this == other);
		}
	};

	class GSCachedTexture
	{
	public:
		const GSMemoryRect& GetRect() const { return m_mem_rect; }
		const GSMemoryCLUT& GetCLUT() const { return m_mem_clut; }
		bool HasCLUT() const { return GSLocalMemory::m_psm[m_mem_rect.PSM].pal; }
		bool Matches(const GSMemoryRect& rect, const GSMemoryCLUT& clut = {}) const;
		bool IsInvalid() { return m_state == TextureState::Invalid; }
		bool IsDirty()   { return m_state == TextureState::Dirty; }
		bool IsClean()   { return m_state == TextureState::Clean; }
		void WritebackIfDirty();

	private:
		friend class GSTextureCacheCompute;
		friend class std::unique_ptr<GSCachedTexture>;
		friend struct std::default_delete<GSCachedTexture>;

		GSCachedTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut = {});
		~GSCachedTexture();
		
		std::unique_ptr<GSTexture> m_texture;
		std::unique_ptr<GSTexture> m_clut;
		GSMemoryRect m_mem_rect;
		GSMemoryCLUT m_mem_clut;
		TextureState m_state;
	};

	GSTextureCacheCompute();

	GSCachedTexture* LookupTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut = {});

	void WritebackAllTextures();

	void WritebackTexture(GSCachedTexture* tex);
	void DestroyTexture(GSCachedTexture* tex);

	// TODO:
	void InvalidatePages(u32 start_page, u32 num_pages, u32 channel_mask);
	void InvalidateBlocks(u32 start_bp, u32 num_blocks, u32 channel_mask);
	void InvalidateRect(u32 start_bp, u32 bw, const GSVector4i& rect, u32 channel_mask);
private:
	std::vector<std::unique_ptr<GSCachedTexture>> m_cache;
};

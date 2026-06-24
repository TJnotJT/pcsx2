// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS/Renderers/Common/GSTexture.h"
#include "GS/Renderers/Common/GSRenderer.h"

#include <unordered_set>
#include <utility>
#include <limits>

// Defined in 
#ifdef PSM
#undef PSM
#endif

static constexpr inline bool IsCLUTPSM(u32 PSM)
{
	return GSLocalMemory::m_psm[PSM].pal;
}

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

		GSMemoryRect(u32 BP, u32 BW, u32 PSM, const GSVector4i& rect)
			: BP(BP), BW(BW), PSM(PSM), rect(rect)
		{
		}

		GSMemoryRect(const GIFRegTEX0& tex0)
			: GSMemoryRect(tex0.TBP0, tex0.TBW, tex0.PSM, GSVector4i(0, 0, 1 << tex0.TW, 1 << tex0.TH))
		{
		}

		GSMemoryRect(const GIFRegFRAME& frame, int height)
			: GSMemoryRect(frame.Block(), frame.FBW, frame.PSM, GSVector4i(0, 0, 64 * frame.FBW, height))
		{
		}

		GSMemoryRect(const GIFRegZBUF& zbuf, const GIFRegFRAME& frame, int height)
			: GSMemoryRect(zbuf.Block(), frame.FBW, zbuf.PSM, GSVector4i(0, 0, 64 * frame.FBW, height))
		{
		}

		int GetX() const { return rect.x; }
		int GetY() const { return rect.y; }
		int GetWidth() const { return rect.width(); }
		int GetHeight() const { return rect.height(); }
		GSVector4i GetCoordRect() const { return rect; }
		GSVector2i GetSize() const { return GSVector2i(GetWidth(), GetHeight()); }

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

		GSMemoryCLUT(u32 PSM, u32 CBP, u32 CBW, u32 CPSM, u32 CSM, u32 COU, u32 COV)
			: PSM(PSM), CBP(CBP), CBW(CBW), CPSM(CPSM), CSM(CSM), COU(COU), COV(COV)
		{
		}

		GSMemoryCLUT() : GSMemoryCLUT(0, 0, 0, 0, 0, 0, 0)
		{
		}

		GSMemoryCLUT(const GIFRegTEX0& tex0, const GIFRegTEXCLUT& texclut)
			: PSM(tex0.PSM), CBP(tex0.CBP), CBW(texclut.CBW), CPSM(tex0.CPSM), CSM(tex0.CSM)
			, COU(texclut.COU), COV(texclut.COV)
		{
		}

		GSMemoryCLUT(const GIFRegTEX0& tex0)
			: PSM(tex0.PSM), CBP(tex0.CBP), CBW(0), CPSM(tex0.CPSM), CSM(tex0.CSM)
			, COU(0), COV(0)
		{
		}

		static GSMemoryCLUT Create(const GIFRegTEX0& tex0, const GIFRegTEXCLUT& texclut)
		{
			if (!IsCLUTPSM(tex0.PSM))
				return GSMemoryCLUT();
			if (tex0.CSM == 1)
				return GSMemoryCLUT(tex0, texclut); // CSM2; all fields are needed.
			else
				return GSMemoryCLUT(tex0); // CSM1; only TEX0 fields are needed.
		}

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
		int GetX() const { return GetRect().GetX(); }
		int GetY() const { return GetRect().GetY(); }
		int GetWidth() const { return GetRect().GetWidth(); }
		int GetHeight() const { return GetRect().GetHeight(); }
		GSVector4i GetCoordRect() const { return GetRect().GetCoordRect(); }
		GSVector2i GetSize() const { return GetRect().GetSize(); }

		const GSMemoryCLUT& GetCLUT() const { return m_mem_clut; }
		bool HasCLUT() const { return GSLocalMemory::m_psm[m_mem_rect.PSM].pal; }

		bool Matches(const GSMemoryRect& rect, const GSMemoryCLUT& clut = GSMemoryCLUT()) const;
		bool IsInvalid() { return m_state == TextureState::Invalid; }
		bool IsDirty()   { return m_state == TextureState::Dirty; }
		bool IsClean()   { return m_state == TextureState::Clean; }
		void WritebackIfDirty();

		GSTexture* GetTexture() { return m_texture.get(); }
		GSTexture* GetCLUT() { return m_clut.get(); }

	private:
		friend class GSTextureCacheCompute;
		friend class std::unique_ptr<GSCachedTexture>;
		friend struct std::default_delete<GSCachedTexture>;

		GSCachedTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut = GSMemoryCLUT());
		~GSCachedTexture();
		
		std::unique_ptr<GSTexture> m_texture;
		std::unique_ptr<GSTexture> m_clut;
		GSMemoryRect m_mem_rect;
		GSMemoryCLUT m_mem_clut;
		TextureState m_state;
	};

	GSTextureCacheCompute();

	GSCachedTexture* LookupTexture(const GSMemoryRect& rect, const GSMemoryCLUT& clut = GSMemoryCLUT());

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

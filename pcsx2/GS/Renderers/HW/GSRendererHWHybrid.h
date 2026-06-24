// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GSTextureCacheHybrid.h"
#include "GS/Renderers/Common/GSRenderer.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GS/GSState.h"
#include "GS/MultiISA.h"

class GSRendererHWHybrid : public GSRenderer
{
protected:
	virtual GSTexture* GetOutput(int i, float& scale, int& y_offset) override;
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) override;
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false) override;

private:
	std::unique_ptr<GSTextureCacheHybrid> m_texture_cache;

	std::vector<GSHybridVertex> m_draw_vertices; // Vertices for the current draw

	void ConvertVertices(GSVertex* input, u32 count, std::vector<GSHybridVertex>& output);
	GSHybridDrawMesh GetDrawMesh();
public:
	using GSCachedTexture = GSTextureCacheHybrid::GSCachedTexture;
	using GSMemoryRect = GSTextureCacheHybrid::GSMemoryRect;
	using GSMemoryCLUT = GSTextureCacheHybrid::GSMemoryCLUT;
	
	GSRendererHWHybrid();
	virtual ~GSRendererHWHybrid() override;

	struct CachedDrawTextures
	{
		GSCachedTexture* rt = nullptr;
		GSCachedTexture* depth = nullptr;
		GSCachedTexture* tex = nullptr;
	};

	CachedDrawTextures LookupCachedDrawTextures();
	GSHybridDrawTextures GetBackendDrawTextures(const CachedDrawTextures& cached);

	u32 GetBackendTopology();
	GSVector4i GetViewport(const GSHybridDrawTextures& textures);

	GSHybridDrawSelector GetSelector(const CachedDrawTextures& textures);
	GSHybridConstantBuffer GetConstantBuffer(const CachedDrawTextures& textures);

	bool IsCoverageAlphaSupported() override;

	void Draw() override;
};

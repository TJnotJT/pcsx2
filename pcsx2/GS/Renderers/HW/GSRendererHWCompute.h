// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GSTextureCacheCompute.h"
#include "GS/Renderers/Common/GSRenderer.h"
#include "GS/GSState.h"
#include "GS/MultiISA.h"

class GSRendererHWCompute : public GSRenderer
{
protected:
	virtual GSTexture* GetOutput(int i, float& scale, int& y_offset) override;
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) override;
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false) override;

private:
	std::unique_ptr<GSTextureCacheCompute> m_texture_cache;

public:
	GSRendererHWCompute();
	virtual ~GSRendererHWCompute() override;
	void Draw() override;
};

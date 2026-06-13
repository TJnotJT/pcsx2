// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Defs.h"

#include <ctime>
#include <string>

class GSPerfMon
{
public:
	enum counter_t
	{
		Prim,
		Draw,
		DrawCalls,
		Readbacks,
		Swizzle,
		Unswizzle,
		Fillrate,
		SyncPoint,
		Barriers,
		RenderPasses,
		DepthCopiesROV, // Overlaps with regular texture copies.
		DrawCallsROV, // Overlaps with regular draw calls.
		BarriersROV, // Overlaps with regular barriers.
		CounterLast,

		// Reused counters for HW.
		TextureCopies = Fillrate,
		TextureUploads = SyncPoint,

		CounterLastHW = CounterLast,
		CounterLastSW = SyncPoint + 1
	};

protected:
	double m_counters[CounterLast] = {};
	double m_stats[CounterLast] = {};
	int m_frame = 0;
	clock_t m_lastframe = 0;
	int m_count = 0;
	int m_disp_fb_sprite_blits = 0;
	bool m_enable_interval = false;
	bool m_collect_interval = false;

	bool CheckInterval()
	{
		return !m_enable_interval || m_collect_interval;
	}

public:
	GSPerfMon();

	void Reset();

	void SetFrame(int frame) { m_frame = frame; }
	int GetFrame() { return m_frame; }
	void EndFrame(bool frame_only);

	void Put(counter_t c, double val)
	{
		if (CheckInterval())
			m_counters[c] += val;
	}
	double GetCounter(counter_t c) { return m_counters[c]; }
	double Get(counter_t c) { return m_stats[c]; }
	void Update();

	void EnableInterval(bool interval)
	{
		m_enable_interval = interval;
	}

	void StartInterval()
	{
		m_collect_interval = true;
	}

	void EndInterval()
	{
		m_collect_interval = false;
	}

	__fi void AddDisplayFramebufferSpriteBlit() { m_disp_fb_sprite_blits++; }
	__fi int GetDisplayFramebufferSpriteBlits()
	{
		const int blits = m_disp_fb_sprite_blits;
		m_disp_fb_sprite_blits = 0;
		return blits;
	}

	GSPerfMon operator-(const GSPerfMon& other);

	void Dump(const std::string& filename, bool hw);
};

extern GSPerfMon g_perfmon;

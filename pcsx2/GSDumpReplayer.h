// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>

namespace GSDumpReplayer
{
	bool IsReplayingDump();

	/// If set, playback will repeat once it reaches the last frame.
	void SetLoopCountStart(s32 loop_count = 0); // For batch mode.
	void SetLoopCount(s32 loop_count = 0);
	int GetLoopCount();
	bool IsRunner();
	bool IsBatchMode();
	void SetIsDumpRunner(bool is_runner);
	void SetIsBatchMode(bool batch_mode);
	void SetDumpGSDataDirHW(const std::string& dir);
	void SetDumpGSDataDirSW(const std::string& dir);

	bool Initialize(const char* filename);
	bool NextDump(); // For batch mode.
	bool ChangeDump(const char* filename);
	void Shutdown();

	std::string GetDumpSerial();
	u32 GetDumpCRC();

	u32 GetFrameNumber();

	void RenderUI();

	static bool ReadDumpFileList(const std::string& dir, std::vector<std::string>& file_list);
} // namespace GSDumpReplayer

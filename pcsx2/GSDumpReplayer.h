// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include <string>
#include <vector>

namespace GSDumpReplayer
{
	bool IsReplayingDump();

	/// If set, playback will repeat once it reaches the last frame.
	void SetLoopCountStart(s32 loop_count = 0); // Batch mode
	void SetLoopCount(s32 loop_count = 0); 
	int GetLoopCount();
	bool IsRunner();
	bool IsBatchMode(); // Batch mode
	void SetIsDumpRunner(bool is_runner, const std::string& name = "");
	std::string GetRunnerName();
	void SetIsBatchMode(bool batch_mode); // Batch mode
	void SetNumBatches(u32 n_batches); // Batch mode
	void SetBatchID(u32 batch_id); // Batch mode
	void SetDumpGSDataDirHW(const std::string& dir); // Batch mode
	void SetDumpGSDataDirSW(const std::string& dir); // Batch mode

	bool Initialize(const char* filename);
	bool NextDump(); // Batch mode
	bool ChangeDumpRegressionTest();
	void EndDumpRegressionTest();
	bool ChangeDump(const char* filename);
	void Shutdown();

	std::string GetDumpSerial();
	u32 GetDumpCRC();

	void SetFrameNumberMax(u32 frame_number_max);
	u32 GetFrameNumber();

	void RenderUI();

	bool GetDumpFileList(const std::string& dir, std::vector<std::string>& file_list, u32 nbatches, u32 batch_id);
} // namespace GSDumpReplayer

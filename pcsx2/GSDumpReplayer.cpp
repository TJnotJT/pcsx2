// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS.h"
#include "GS/GSPerfMon.h"
#include "GS/GSState.h"
#include "GS/GSLzma.h"
#include "GSDumpReplayer.h"
#include "GameList.h"
#include "Gif.h"
#include "Gif_Unit.h"
#include "Host.h"
#include "ImGui/ImGuiManager.h"
#include "ImGui/ImGuiOverlays.h"
#include "R3000A.h"
#include "R5900.h"
#include "VMManager.h"
#include "VUmicro.h"
#include "GSRegressionTester.h"

#include "imgui.h"

#include "fmt/format.h"

#include "common/Error.h"
#include "common/FileSystem.h"
#include "common/Path.h"
#include "common/StringUtil.h"
#include "common/Threading.h"
#include "common/Timer.h"
#include "common/ScopedGuard.h"

#include <filesystem>
#include <atomic>

void GSDumpReplayerCpuReserve();
void GSDumpReplayerCpuShutdown();
void GSDumpReplayerCpuReset();
void GSDumpReplayerCpuStep();
void GSDumpReplayerCpuExecute();
void GSDumpReplayerExitExecution();
void GSDumpReplayerCancelInstruction();
void GSDumpReplayerCpuClear(u32 addr, u32 size);

static std::unique_ptr<GSDumpFile> s_dump_file;
static std::string s_runner_name;
static std::string s_dump_dir;
static std::string s_dump_gs_data_dir_hw;
static std::string s_dump_gs_data_dir_sw;
static std::string s_dump_name;
static std::vector<std::string> s_dump_file_list;
static std::size_t s_curr_dump = 0;
static u32 s_current_packet = 0;
static u32 s_dump_frame_number_max = 0;
static u32 s_dump_frame_number = 0;
static s32 s_dump_loop_count_start = 0;
static s32 s_dump_loop_count = 0;
static bool s_dump_running = false;
static bool s_needs_state_loaded = false;
static u64 s_frame_ticks = 0;
static u64 s_next_frame_time = 0;
static bool s_is_dump_runner = false;
static bool s_batch_mode = false;
static u32 s_num_batches = 1;
static u32 s_batch_id = 0;

R5900cpu GSDumpReplayerCpu = {
	GSDumpReplayerCpuReserve,
	GSDumpReplayerCpuShutdown,
	GSDumpReplayerCpuReset,
	GSDumpReplayerCpuStep,
	GSDumpReplayerCpuExecute,
	GSDumpReplayerExitExecution,
	GSDumpReplayerCancelInstruction,
	GSDumpReplayerCpuClear};

static InterpVU0 gsDumpVU0;
static InterpVU1 gsDumpVU1;

bool GSDumpReplayer::IsReplayingDump()
{
	return static_cast<bool>(s_dump_file) || (IsRunner() && IsBatchMode());
}

bool GSDumpReplayer::IsRunner()
{
	return s_is_dump_runner;
}

bool GSDumpReplayer::IsBatchMode()
{
	return s_batch_mode;
}

void GSDumpReplayer::SetIsDumpRunner(bool is_runner, const std::string& runner_name)
{
	s_is_dump_runner = is_runner;
	if (is_runner)
		s_runner_name = runner_name;
}

std::string GSDumpReplayer::GetRunnerName()
{
	return s_runner_name;
}

std::string GSDumpReplayer::GetDumpName()
{
	return s_dump_name;
}

void GSDumpReplayer::SetIsBatchMode(bool batch_mode)
{
	s_batch_mode = batch_mode;
}

void GSDumpReplayer::SetNumBatches(u32 n_batches)
{
	s_num_batches = n_batches;
}

void GSDumpReplayer::SetBatchID(u32 batch_id)
{
	s_batch_id = batch_id;
}

void GSDumpReplayer::SetDumpGSDataDirHW(const std::string& dir)
{
	s_dump_gs_data_dir_hw = dir;
}

void GSDumpReplayer::SetDumpGSDataDirSW(const std::string& dir)
{
	s_dump_gs_data_dir_sw = dir;
}

void GSDumpReplayer::SetLoopCount(s32 loop_count)
{
	s_dump_loop_count = loop_count - 1;
}

void GSDumpReplayer::SetLoopCountStart(s32 loop_count)
{
	s_dump_loop_count_start = loop_count;
}

int GSDumpReplayer::GetLoopCount()
{
	return s_dump_loop_count;
}

void GSDumpReplayer::EndDumpRegressionTest()
{
	pxAssert(GSIsRegressionTesting());

	GSRegressionBuffer* rbp = GSGetRegressionBuffer();

	rbp->SetStateRunner(GSRegressionBuffer::WRITE_DATA);
	ScopedGuard set_default([&]() {
		rbp->SetStateRunner(GSRegressionBuffer::DEFAULT);
	});

	// Note: must process only one packet sequentially or it will break ring buffer locking.

	if (GSIsHardwareRenderer())
	{
		// Send HW stats packet
		GSRegressionPacket* packet_hwstat = nullptr;
		ScopedGuard done_hwstat([&]() {
			if (packet_hwstat)
				rbp->DonePacketWrite();
		});

		if (packet_hwstat = rbp->GetPacketWrite(true))
		{
			const std::string name_dump = rbp->GetNameDump();
			packet_hwstat->SetNameDump(name_dump);
			packet_hwstat->SetNamePacket(name_dump + " HWStat");

			GSRegressionPacket::HWStat hwstat;
			hwstat.frames = 0; // FIXME
			hwstat.draws = g_perfmon.GetCounter(GSPerfMon::DrawCalls);
			hwstat.render_passes = g_perfmon.GetCounter(GSPerfMon::RenderPasses);
			hwstat.barriers = g_perfmon.GetCounter(GSPerfMon::Barriers);
			hwstat.copies = g_perfmon.GetCounter(GSPerfMon::TextureCopies);
			hwstat.uploads = g_perfmon.GetCounter(GSPerfMon::TextureUploads);
			hwstat.readbacks = g_perfmon.GetCounter(GSPerfMon::Readbacks);
			packet_hwstat->SetHWStat(hwstat);

			Console.WriteLnFmt("(GSDumpReplayer/{}) New regression packet: {} / {}",
				GSDumpReplayer::GetRunnerName(), packet_hwstat->GetNameDump(), packet_hwstat->GetNamePacket());
		}
		else
		{
			Console.ErrorFmt("(GSDumpReplayer/{}) Failed to get regression packet for HW stats.", GSDumpReplayer::GetRunnerName());
		}
	}

	{
		// Send done dump packet.
		GSRegressionPacket* packet_done_dump = nullptr;
		ScopedGuard done_done_dump([&]() {
			if (packet_done_dump)
				rbp->DonePacketWrite();
		});

		if (packet_done_dump = rbp->GetPacketWrite(true))
		{
			const std::string name_dump = rbp->GetNameDump();
			packet_done_dump->SetNameDump(name_dump);
			packet_done_dump->SetNamePacket(name_dump + " Done");
			packet_done_dump->SetDoneDump();

			Console.WriteLnFmt("(GSDumpReplayer/{}) New regression packet: {} / {}",
				GSDumpReplayer::GetRunnerName(), packet_done_dump->GetNameDump(), packet_done_dump->GetNamePacket());
		}
		else
		{
			Console.ErrorFmt("(GSDumpReplayer/{}) Failed to get regression packet for done dump signal.",
				GSDumpReplayer::GetRunnerName());
		}
	}
}

bool GSDumpReplayer::NextDump()
{
	if (!IsBatchMode())
	{
		Console.ErrorFmt("Called NextDump() while not in batch mode.");
		return false;
	}

	Error error;

	if (GSIsRegressionTesting())
	{
		return ChangeDumpRegressionTest();
	}
	else
	{
		if (s_curr_dump >= s_dump_file_list.size())
			return false;

		if (!ChangeDump(s_dump_file_list[s_curr_dump++].c_str()))
			return false;

		return true;
	}
}

bool GSDumpReplayer::GetDumpFileList(const std::string& dir, std::vector<std::string>& file_list, u32 nbatches, u32 batch_id)
{
	if (nbatches == 0)
	{
		Host::ReportErrorAsync("GSDumpReplayer", fmt::format("nbatches must be positive -- got {}.", nbatches));
		return false;
	}

	file_list.clear();

	if (!FileSystem::DirectoryExists(dir.c_str()))
	{
		Host::ReportErrorAsync("GSDumpReplayer", fmt::format("Directory does not exist: {}", dir));
		return false;
	}

	FileSystem::FindResultsArray files;
	FileSystem::FindFiles(
		dir.c_str(),
		"*",
		FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_HIDDEN_FILES,
		&files);

	std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
		return a.FileName < b.FileName;
	});

	for (u32 i = s_batch_id; i < files.size(); i += s_num_batches)
	{
		if (VMManager::IsGSDumpFileName(files[i].FileName))
			file_list.push_back(files[i].FileName);
	}

	if (file_list.empty())
	{
		Host::ReportErrorAsync("GSDumpReplayer", fmt::format("Could not find any dumps in '{}'", dir));
		return false;
	}

	Console.WriteLnFmt("(GSDumpReplayer) Read a dump file list with {} dumps", file_list.size());

	return true;
}

bool GSDumpReplayer::Initialize(const char* filename)
{
	if (GSIsRegressionTesting())
	{
		if (!NextDump())
			return false;
	}
	else if (IsBatchMode())
	{
		if (!GetDumpFileList(filename, s_dump_file_list, s_num_batches, s_batch_id))
			return false;

		if (!NextDump())
			return false;
	}
	else
	{
		return ChangeDump(filename);
	}

	// We replace all CPUs.
	Cpu = &GSDumpReplayerCpu;
	psxCpu = &psxInt;
	CpuVU0 = &gsDumpVU0;
	CpuVU1 = &gsDumpVU1;

	// loop infinitely by default
	s_dump_loop_count = -1;

	return true;
}

bool GSDumpReplayer::ChangeDumpRegressionTest()
{
	GSRegressionBuffer* rbp = GSGetRegressionBuffer();
	GSDumpFileSharedMemory* dump = nullptr;
	rbp->SetStateRunner(GSRegressionBuffer::WAIT_DUMP);

	ScopedGuard sg([&]() {
		rbp->SetStateRunner(GSRegressionBuffer::DEFAULT);
		if (dump)
			rbp->DoneDumpRead();
	});

	Common::Timer timer;
	while (true)
	{
		// Get state before checking dump buffer to avoid race condition.
		u32 state_tester = rbp->GetStateTester();

		if (dump = rbp->GetDumpRead(false))
			break;

		if (state_tester == GSRegressionBuffer::EXIT)
		{
			Console.WarningFmt("(GSRunner/{}) Got exit state from tester.", GetRunnerName());
			s_dump_running = false;
			return false;
		}

		if (state_tester == GSRegressionBuffer::DONE_UPLOADING)
		{
			Console.WriteLnFmt("(GSRunner/{}) Got done uploading from tester.", GetRunnerName());
			s_dump_running = false;
			return false;
		}

		if (!GSProcess::IsParentRunning())
		{
			Console.ErrorFmt("(GSRunner/{}) Tester process exited.", GetRunnerName());
			s_dump_running = false;
			return false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		if (timer.GetTimeSeconds() >= 1.0)
		{
			Console.WriteLnFmt("(GSDumpReplayer: {}) Waiting for new dump.", s_runner_name);
			timer.Reset();
		}
	}

	const std::string dump_name = dump->GetNameDump();

	rbp->SetNameDump(dump_name);

	s_dump_file = GSDumpFile::OpenGSDumpMemory(dump->GetPtrDump(), dump->GetSizeDump());

	Error error;
	if (!s_dump_file->ReadFile(&error))
	{
		Host::ReportErrorAsync("GSDumpReplayer", fmt::format("Failed to open GS dump from memory: {}", error.GetDescription()));
		return false;
	}

	Console.WriteLnFmt("(GSDumpReplayer) Switching to {}...", dump_name);

	return true;
}

bool GSDumpReplayer::ChangeDump(const char* filename)
{
	Console.WriteLnFmt("(GSDumpReplayer) Switching to {}...", filename);

	if (!VMManager::IsGSDumpFileName(filename))
	{
		Host::ReportFormattedErrorAsync("GSDumpReplayer", "'%s' is not a GS dump.", filename);
		return false;
	}

	Error error;
	std::unique_ptr<GSDumpFile> new_dump(GSDumpFile::OpenGSDump(filename));
	if (!new_dump || !new_dump->ReadFile(&error))
	{
		Host::ReportErrorAsync("GSDumpReplayer", fmt::format("Failed to open or read '{}': {}",
			Path::GetFileName(filename), error.GetDescription()));
		return false;
	}

	s_dump_name = std::filesystem::path(filename).filename().string();
	s_dump_file = std::move(new_dump);

	// Don't forget to reset the GS!
	GSDumpReplayerCpuReset();

	if (IsBatchMode() && GSConfig.DumpGSData)
	{
		// In case we are saving GS data in batch mode, make sure to update the directories.
		std::string* src_dir[] = {&s_dump_gs_data_dir_hw, &s_dump_gs_data_dir_sw};
		std::string* dst_dir[] = {&GSConfig.HWDumpDirectory, &GSConfig.SWDumpDirectory};

		for (int i = 0; i < 2; i++)
		{
			*dst_dir[i] = (std::filesystem::path(*src_dir[i]) / s_dump_name).string();
			if (!FileSystem::EnsureDirectoryExists(dst_dir[i]->c_str(), false, &error))
			{
				Host::ReportErrorAsync("GSDumpReplayer", fmt::format("(GSDumpReplayer) Could not create output directory: {} ({})", *dst_dir[i], error.GetDescription()));
			}
			else
			{
				Console.WriteLnFmt("(GSDumpReplayer) Dumping GS data to '{}'", *dst_dir[i]);
			}
		}
	}

	return true;
}

void GSDumpReplayer::Shutdown()
{
	Console.WriteLn("(GSDumpReplayer) Shutting down.");

	Cpu = nullptr;
	psxCpu = nullptr;
	CpuVU0 = nullptr;
	CpuVU1 = nullptr;
	s_dump_file.reset();
}

std::string GSDumpReplayer::GetDumpSerial()
{
	if (IsBatchMode())
		return "";

	std::string ret;

	if (!s_dump_file->GetSerial().empty())
	{
		ret = s_dump_file->GetSerial();
	}
	else if (s_dump_file->GetCRC() != 0)
	{
		// old dump files don't have serials, but we have the crc...
		// so, let's try searching the game list for a crc match.
		auto lock = GameList::GetLock();
		const GameList::Entry* entry = GameList::GetEntryByCRC(s_dump_file->GetCRC());
		if (entry)
			ret = entry->serial;
	}

	return ret;
}

u32 GSDumpReplayer::GetDumpCRC()
{
	return IsBatchMode() ? 0 : s_dump_file->GetCRC();
}

void GSDumpReplayer::SetFrameNumberMax(u32 frame_number_max)
{
	s_dump_frame_number_max = frame_number_max;
}

u32 GSDumpReplayer::GetFrameNumber()
{
	return s_dump_frame_number;
}

void GSDumpReplayerCpuReserve()
{
}

void GSDumpReplayerCpuShutdown()
{
}

void GSDumpReplayerCpuReset()
{
	s_needs_state_loaded = true;
	s_current_packet = 0;
	s_dump_frame_number = 0;
}

static void GSDumpReplayerLoadInitialState()
{
	// reset GS registers to initial dump values
	std::memcpy(PS2MEM_GS, s_dump_file->GetRegsData().data(),
		std::min(Ps2MemSize::GSregs, static_cast<u32>(s_dump_file->GetRegsData().size())));

	// load GS state
	freezeData fd = {static_cast<int>(s_dump_file->GetStateData().size()),
		const_cast<u8*>(s_dump_file->GetStateData().data())};
	MTGS::FreezeData mfd = {&fd, 0};
	MTGS::Freeze(FreezeAction::Load, mfd);
	if (mfd.retval != 0)
		Host::ReportFormattedErrorAsync("GSDumpReplayer", "Failed to load GS state.");
}

static void GSDumpReplayerSendPacketToMTGS(GIF_PATH path, const u8* data, u32 length)
{
	pxAssert((length % 16) == 0);

	Gif_Path& gifPath = gifUnit.gifPath[path];
	gifPath.CopyGSPacketData(const_cast<u8*>(data), length);

	GS_Packet gsPack;
	gsPack.offset = gifPath.curOffset;
	gsPack.size = length;
	gifPath.curOffset += length;
	Gif_AddCompletedGSPacket(gsPack, path);
}

static void GSDumpReplayerUpdateFrameLimit()
{
	constexpr u32 default_frame_limit = 60;
	const u32 frame_limit = static_cast<u32>(default_frame_limit * VMManager::GetTargetSpeed());

	if (frame_limit > 0)
		s_frame_ticks = (GetTickFrequency() + (frame_limit / 2)) / frame_limit;
	else
		s_frame_ticks = 0;
}

static void GSDumpReplayerFrameLimit()
{
	if (s_frame_ticks == 0)
		return;

	// Frame limiter
	u64 now = GetCPUTicks();
	const s64 ms = GetTickFrequency() / 1000;
	const s64 sleep = s_next_frame_time - now - ms;
	if (sleep > ms)
		Threading::Sleep(sleep / ms);
	while ((now = GetCPUTicks()) < s_next_frame_time)
		ShortSpin();
	s_next_frame_time = std::max(now, s_next_frame_time + s_frame_ticks);
}

void GSDumpReplayerCpuStep()
{
	if (s_needs_state_loaded)
	{
		GSDumpReplayerLoadInitialState();
		s_needs_state_loaded = false;
	}

	const GSDumpFile::GSData& packet = s_dump_file->GetPackets()[s_current_packet];

	switch (packet.id)
	{
		case GSDumpTypes::GSType::Transfer:
		{
			switch (packet.path)
			{
				case GSDumpTypes::GSTransferPath::Path1Old:
				{
					std::unique_ptr<u8[]> data(new u8[16384]);
					const s32 addr = 16384 - packet.length;
					std::memcpy(data.get(), packet.data + addr, packet.length);
					GSDumpReplayerSendPacketToMTGS(GIF_PATH_1, data.get(), packet.length);
				}
				break;

				case GSDumpTypes::GSTransferPath::Path1New:
				case GSDumpTypes::GSTransferPath::Path2:
				case GSDumpTypes::GSTransferPath::Path3:
				{
					GSDumpReplayerSendPacketToMTGS(static_cast<GIF_PATH>(static_cast<u8>(packet.path) - 1),
						packet.data, packet.length);
				}
				break;

				default:
					break;
			}
			break;
		}

		case GSDumpTypes::GSType::VSync:
		{
			Host::PumpMessagesOnCPUThread(); // Update frame number.
			GSDumpReplayerUpdateFrameLimit();
			GSDumpReplayerFrameLimit();
			MTGS::PostVsyncStart(false);
			VMManager::Internal::VSyncOnCPUThread();
			if (VMManager::Internal::IsExecutionInterrupted())
				GSDumpReplayerExitExecution();
		}
		break;

		case GSDumpTypes::GSType::ReadFIFO2:
		{
			u32 size;
			std::memcpy(&size, packet.data, sizeof(size));

			// Allocate an extra quadword, some transfers write too much (e.g. Lego Racers 2 with Z24 downloads).
			std::unique_ptr<u8[]> arr(new u8[(size + 1) * 16]);
			MTGS::InitAndReadFIFO(arr.get(), size);
		}
		break;

		case GSDumpTypes::GSType::Registers:
		{
			std::memcpy(PS2MEM_GS, packet.data, std::min<s32>(packet.length, Ps2MemSize::GSregs));
		}
		break;
	}

	// Handle state.

	bool done_all_dumps = false;
	bool done_dump = false;

	s_current_packet = (s_current_packet + 1) % static_cast<u32>(s_dump_file->GetPackets().size());
	if (s_current_packet == 0)
	{
		s_dump_frame_number = 0;
		if (s_dump_loop_count > 0)
			s_dump_loop_count--;
		else
			done_dump = true;
		
	}
	else if (packet.id == GSDumpTypes::GSType::VSync)
	{
		s_dump_frame_number++;
	}

	done_dump = done_dump || (s_dump_frame_number_max > 0 && s_dump_frame_number >= s_dump_frame_number_max);

	if (GSIsRegressionTesting() && GSGetRegressionBuffer()->GetStateTester() == GSRegressionBuffer::EXIT)
	{
		done_all_dumps = true;
	}
	else if (done_dump)
	{
		// Check if we need to change dumps for batch mode; or done with all dumps.

		if (GSDumpReplayer::IsBatchMode())
		{
			Host::OnDumpChanged(); // Dump stats
			MTGS::ResetGS(true);
			MTGS::WaitGS(false, false, false); // Let GS thread finish.
			GSState::s_n = 0; // Needed for proper file naming for next dump. 

			// Send HW stats and done packet if needed.
			if (GSIsRegressionTesting())
			{
				GSDumpReplayer::EndDumpRegressionTest();
			}

			if (GSDumpReplayer::NextDump())
			{
				std::string n = GSDumpReplayer::GetDumpName();
				GSDumpReplayer::SetLoopCount(s_dump_loop_count_start);
				GSDumpReplayerCpuReset();
			}
			else
			{
				Console.WriteLnFmt("(GSDumpReplayer/{}) Batch mode has no more dumps.", GSDumpReplayer::GetRunnerName());
				done_all_dumps = true;
			}
		}
		else // Normal (non-batch) mode
		{
			done_all_dumps = true;
		}
	}

	if (done_all_dumps)
	{
		Host::RequestVMShutdown(false, false, false);
		GSDumpReplayerExitExecution();
	}
}

void GSDumpReplayerCpuExecute()
{
	s_dump_running = true;
	s_next_frame_time = GetCPUTicks();

	while (s_dump_running)
	{
		GSDumpReplayerCpuStep();
	}
}

void GSDumpReplayerExitExecution()
{
	s_dump_running = false;
}

void GSDumpReplayerCancelInstruction()
{
}

void GSDumpReplayerCpuClear(u32 addr, u32 size)
{
}

void GSDumpReplayer::RenderUI()
{
	const float scale = ImGuiManager::GetGlobalScale();
	const float shadow_offset = std::ceil(1.0f * scale);
	const float margin = std::ceil(10.0f * scale);
	const float spacing = std::ceil(5.0f * scale);
	float position_y = margin;

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	ImFont* const font = ImGuiManager::GetFixedFont();
	const float font_size = ImGuiManager::GetFontSizeStandard();

	std::string text;
	ImVec2 text_size;
	text.reserve(128);

#define DRAW_LINE(font, size, text, color) \
	do \
	{ \
		text_size = font->CalcTextSizeA(size, std::numeric_limits<float>::max(), -1.0f, (text), nullptr, nullptr); \
		const ImVec2 text_pos = CalculatePerformanceOverlayTextPosition(GSConfig.OsdMessagesPos, margin, text_size, ImGuiManager::GetWindowWidth(), position_y); \
		dl->AddText(font, size, ImVec2(text_pos.x + shadow_offset, text_pos.y + shadow_offset), IM_COL32(0, 0, 0, 100), (text)); \
		dl->AddText(font, size, text_pos, color, (text)); \
		position_y += text_size.y + spacing; \
	} while (0)

	fmt::format_to(std::back_inserter(text), "Dump Frame: {}", s_dump_frame_number);
	DRAW_LINE(font, font_size, text.c_str(), IM_COL32(255, 255, 255, 255));

	text.clear();
	fmt::format_to(std::back_inserter(text), "Packet Number: {}/{}", s_current_packet, static_cast<u32>(s_dump_file->GetPackets().size()));
	DRAW_LINE(font, font_size, text.c_str(), IM_COL32(255, 255, 255, 255));

#undef DRAW_LINE
}

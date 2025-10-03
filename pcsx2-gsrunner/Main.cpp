// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include "common/RedtapeWindows.h"
#endif

#include "fmt/format.h"

#include "common/Error.h"
#include "common/Timer.h"
#include "common/Assertions.h"
#include "common/CocoaTools.h"
#include "common/Console.h"
#include "common/CrashHandler.h"
#include "common/FileSystem.h"
#include "common/MemorySettingsInterface.h"
#include "common/Path.h"
#include "common/ProgressCallback.h"
#include "common/SettingsWrapper.h"
#include "common/StringUtil.h"
#include "common/ScopedGuard.h"

#include "pcsx2/PrecompiledHeader.h"

#include "pcsx2/Achievements.h"
#include "pcsx2/CDVD/CDVD.h"
#include "pcsx2/GS.h"
#include "pcsx2/GS/GSLzma.h"
#include "pcsx2/GS/GSPerfMon.h"
#include "pcsx2/GSDumpReplayer.h"
#include "pcsx2/GameList.h"
#include "pcsx2/Host.h"
#include "pcsx2/INISettingsInterface.h"
#include "pcsx2/ImGui/FullscreenUI.h"
#include "pcsx2/ImGui/ImGuiFullscreen.h"
#include "pcsx2/ImGui/ImGuiManager.h"
#include "pcsx2/Input/InputManager.h"
#include "pcsx2/MTGS.h"
#include "pcsx2/SIO/Pad/Pad.h"
#include "pcsx2/PerformanceMetrics.h"
#include "pcsx2/VMManager.h"

#include "pcsx2/GSRegressionTester.h"
#include "pcsx2/GS/GSPng.h"
#include "pcsx2/GS/GSLzma.h"

#include "svnrev.h"

// For writing YAML files.
static constexpr const char* INDENT = "    ";
static constexpr const char* OPEN_MAP = "{";
static constexpr const char* CLOSE_MAP = "}";
static constexpr const char* QUOTE = "\"";
static constexpr const char* KEY_VAL_DEL = ": ";
static constexpr const char* LIST_DEL = ", ";
static constexpr const char* LIST_ITEM = "- ";
static constexpr const char* OPEN_LIST = "[";
static constexpr const char* CLOSE_LIST = "]";

struct GSTester
{
	struct DumpInfo
	{
		enum State
		{
			UNVISITED,
			COMPLETED,
			SKIPPED
		};

		std::string file;
		std::string name;
		std::size_t packets_skipped = 0;
		std::size_t packets_completed = 0;
		State state = UNVISITED;

		DumpInfo(const std::string& file, const std::string& name)
			: file(file)
			, name(name)
		{
		}
	};

	enum ReturnValue
	{
		SUCCESS,
		BUFFER_NOT_READY,
		ERROR_
	};

	struct DumpCached
	{
		std::unique_ptr<GSDumpFile> ptr;
		std::string name;

		bool Load(DumpInfo& d, std::size_t max_dump_size, Error* error);
		bool HasCached();
		void Reset();
	};

	static void PrintCommandLineHelp(const char* progname);
	bool ParseCommandLineArgs(int argc, char* argv[], u32 thread_id);
	bool GetDumpInfo(u32 nthreads, u32 thread_id);
	ReturnValue CopyDumpToSharedMemory(const std::unique_ptr<GSDumpFile>& dump, const std::string& name);
	ReturnValue ProcessPackets();
	bool StartRunners();
	bool EndRunners();
	bool RestartRunners();
	int MainThread(int argc, char* argv[], u32 nthreads = 1, u32 thread_id = 0);
	
	static int main_tester(int argc, char* argv[]);

	enum
	{
		VERBOSE_LOW,
		VERBOSE_TESTER,
		VERBOSE_TESTER_AND_RUNNER
	};

	static constexpr std::size_t regression_deadlock_timeout = 1000;
	static constexpr std::size_t regression_failure_restarts_max = 10;
	static constexpr std::size_t regression_dump_size_default = 256 * _1mb;
	static constexpr std::size_t regression_num_packets_default = 10;
	static constexpr u32 regression_verbose_level_default = VERBOSE_LOW;

	std::vector<DumpInfo> regression_dumps;
	std::map<std::string, std::size_t> regression_dumps_map;
	std::string regression_dump_last_completed;
	std::string regression_output_dir;
	std::string regression_output_image_dir[2];
	std::string regression_output_hwstat_dir[2];
	std::string regression_runner_args;
	GSRegressionBuffer regression_buffer[2];
	std::string regression_runner_path[2];
	std::string regression_runner_command[2];
	std::string regression_runner_name[2];
	std::string regression_shared_file[2];
	std::string regression_dump_dir;
	GSProcess regression_runner_proc[2];
	std::size_t regression_num_packets = regression_num_packets_default;
	std::size_t regression_dump_size = regression_dump_size_default;
	u32 regression_verbose_level = regression_verbose_level_default;

	// Stats
	std::size_t regression_packets_completed = 0;
	std::size_t regression_packets_skipped = 0;
	std::size_t regression_dumps_completed = 0;
	std::size_t regression_dumps_skipped = 0;
	std::size_t regression_dumps_unvisited = 0;
	std::size_t regression_failure_restarts = 0;
	std::size_t regression_packets_skipped_unknown = 0;
};

namespace GSRunner
{
	static void InitializeConsole();
	static bool InitializeConfig();
	static bool ParseCommandLineArgs(int argc, char* argv[], VMBootParameters& params);
	static void PrintCommandLineHelp(const char* progname);
	static void DumpStats();

	static bool CreatePlatformWindow();
	static void DestroyPlatformWindow();
	static std::optional<WindowInfo> GetPlatformWindowInfo();
	static void PumpPlatformMessages(bool forever = false);
	static void StopPlatformMessagePump();

	int main_runner(int argc, char* argv[]);

	static constexpr u32 WINDOW_WIDTH = 640;
	static constexpr u32 WINDOW_HEIGHT = 480;

	static MemorySettingsInterface s_settings_interface;

	static std::string s_output_prefix;
	static std::string s_runner_name;
	static std::string s_regression_file;
	GSRegressionBuffer s_regression_buffer;
	static size_t s_regression_num_packets = GSTester::regression_num_packets_default;
	static size_t s_regression_dump_size = GSTester::regression_dump_size_default;
	static s32 s_loop_count = 1;
	static std::optional<bool> s_use_window;
	static bool s_no_console = false;
	static bool s_batch_mode = false;
	static u32 s_num_batches = 1;
	static u32 s_batch_id = 0;
	static u32 s_frames_max = 0xFFFFFFFF;
	static u32 s_parent_pid = 0;

	// Owned by the GS thread.
	static u32 s_dump_frame_number = 0;
	static u32 s_loop_number = s_loop_count;
	static double s_last_internal_draws = 0;
	static double s_last_draws = 0;
	static double s_last_render_passes = 0;
	static double s_last_barriers = 0;
	static double s_last_copies = 0;
	static double s_last_uploads = 0;
	static double s_last_readbacks = 0;
	static u64 s_total_internal_draws = 0;
	static u64 s_total_draws = 0;
	static u64 s_total_render_passes = 0;
	static u64 s_total_barriers = 0;
	static u64 s_total_copies = 0;
	static u64 s_total_uploads = 0;
	static u64 s_total_readbacks = 0;
	static u32 s_total_frames = 0;
	static u32 s_total_drawn_frames = 0;
	static std::string s_dump_gs_data_dir_hw;
	static std::string s_dump_gs_data_dir_sw;
} // namespace GSRunner

bool GSRunner::InitializeConfig()
{
	EmuFolders::SetAppRoot();
	if (!EmuFolders::SetResourcesDirectory() || !EmuFolders::SetDataDirectory(nullptr))
		return false;

	CrashHandler::SetWriteDirectory(EmuFolders::DataRoot);

	const char* error;
	if (!VMManager::PerformEarlyHardwareChecks(&error))
		return false;

	ImGuiManager::SetFontPath(Path::Combine(EmuFolders::Resources, "fonts" FS_OSPATH_SEPARATOR_STR "Roboto-Regular.ttf"));

	// don't provide an ini path, or bother loading. we'll store everything in memory.
	MemorySettingsInterface& si = s_settings_interface;
	Host::Internal::SetBaseSettingsLayer(&si);

	VMManager::SetDefaultSettings(si, true, true, true, true, true);

	// complete as quickly as possible
	si.SetBoolValue("EmuCore/GS", "FrameLimitEnable", false);
	si.SetIntValue("EmuCore/GS", "VsyncEnable", false);

	// Force screenshot quality settings to something more performant, overriding any defaults good for users.
	si.SetIntValue("EmuCore/GS", "ScreenshotFormat", static_cast<int>(GSScreenshotFormat::PNG));
	si.SetIntValue("EmuCore/GS", "ScreenshotQuality", 10);

	// ensure all input sources are disabled, we're not using them
	si.SetBoolValue("InputSources", "SDL", false);
	si.SetBoolValue("InputSources", "XInput", false);

	// we don't need any sound output
	si.SetStringValue("SPU2/Output", "OutputModule", "nullout");

	// none of the bindings are going to resolve to anything
	Pad::ClearPortBindings(si, 0);
	si.ClearSection("Hotkeys");

	// force logging
	si.SetBoolValue("Logging", "EnableSystemConsole", !s_no_console);
	si.SetBoolValue("Logging", "EnableTimestamps", true);
	si.SetBoolValue("Logging", "EnableVerbose", true);

	// and show some stats :)
	si.SetBoolValue("EmuCore/GS", "OsdShowFPS", true);
	si.SetBoolValue("EmuCore/GS", "OsdShowResolution", true);
	si.SetBoolValue("EmuCore/GS", "OsdShowGSStats", true);

	// remove memory cards, so we don't have sharing violations
	for (u32 i = 0; i < 2; i++)
	{
		si.SetBoolValue("MemoryCards", fmt::format("Slot{}_Enable", i + 1).c_str(), false);
		si.SetStringValue("MemoryCards", fmt::format("Slot{}_Filename", i + 1).c_str(), "");
	}

	VMManager::Internal::LoadStartupSettings();
	return true;
}

void Host::CommitBaseSettingChanges()
{
	// nothing to save, we're all in memory
}

void Host::LoadSettings(SettingsInterface& si, std::unique_lock<std::mutex>& lock)
{
}

void Host::CheckForSettingsChanges(const Pcsx2Config& old_config)
{
}

bool Host::RequestResetSettings(bool folders, bool core, bool controllers, bool hotkeys, bool ui)
{
	// not running any UI, so no settings requests will come in
	return false;
}

void Host::SetDefaultUISettings(SettingsInterface& si)
{
	// nothing
}

bool Host::LocaleCircleConfirm()
{
	// not running any UI, so no settings requests will come in
	return false;
}

std::unique_ptr<ProgressCallback> Host::CreateHostProgressCallback()
{
	return ProgressCallback::CreateNullProgressCallback();
}

void Host::ReportInfoAsync(const std::string_view title, const std::string_view message)
{
	if (!title.empty() && !message.empty())
		INFO_LOG("ReportInfoAsync: {}: {}", title, message);
	else if (!message.empty())
		INFO_LOG("ReportInfoAsync: {}", message);
}

void Host::ReportErrorAsync(const std::string_view title, const std::string_view message)
{
	if (!title.empty() && !message.empty())
		ERROR_LOG("ReportErrorAsync: {}: {}", title, message);
	else if (!message.empty())
		ERROR_LOG("ReportErrorAsync: {}", message);
}

bool Host::ConfirmMessage(const std::string_view title, const std::string_view message)
{
	if (!title.empty() && !message.empty())
		ERROR_LOG("ConfirmMessage: {}: {}", title, message);
	else if (!message.empty())
		ERROR_LOG("ConfirmMessage: {}", message);

	return true;
}

void Host::OpenURL(const std::string_view url)
{
	// noop
}

bool Host::CopyTextToClipboard(const std::string_view text)
{
	return false;
}

void Host::BeginTextInput()
{
	// noop
}

void Host::EndTextInput()
{
	// noop
}

std::optional<WindowInfo> Host::GetTopLevelWindowInfo()
{
	return GSRunner::GetPlatformWindowInfo();
}

void Host::OnInputDeviceConnected(const std::string_view identifier, const std::string_view device_name)
{
}

void Host::OnInputDeviceDisconnected(const InputBindingKey key, const std::string_view identifier)
{
}

void Host::SetMouseMode(bool relative_mode, bool hide_cursor)
{
}

std::optional<WindowInfo> Host::AcquireRenderWindow(bool recreate_window)
{
	return GSRunner::GetPlatformWindowInfo();
}

void Host::ReleaseRenderWindow()
{
}

void Host::BeginPresentFrame()
{
	if (GSRunner::s_loop_number == 0 && !GSRunner::s_output_prefix.empty())
	{
		// when we wrap around, don't race other files
		GSJoinSnapshotThreads();

		std::string prefix = GSRunner::s_output_prefix;

		if (GSRunner::s_batch_mode)
		{
			std::string dump_name = GSDumpReplayer::GetDumpName();
			std::string_view title(Path::GetFileTitle(dump_name));
			if (StringUtil::EndsWithNoCase(title, ".gs"))
				title = Path::GetFileTitle(title);
			prefix = Path::Combine(prefix, StringUtil::StripWhitespace(title));

			if (title.starts_with("Final"))
			{
				printf("");
			}
		}

		// queue dumping of this frame
		std::string dump_file(fmt::format("{}_frame{:05}.png", prefix, GSRunner::s_dump_frame_number));
		GSQueueSnapshot(dump_file);
	}

	if (GSIsHardwareRenderer())
	{
		const u32 last_draws = GSRunner::s_total_internal_draws;
		const u32 last_uploads = GSRunner::s_total_uploads;

		static constexpr auto update_stat = [](GSPerfMon::counter_t counter, u64& dst, double& last) {
			// perfmon resets every 30 frames to zero
			const double val = g_perfmon.GetCounter(counter);
			dst += static_cast<u64>((val < last) ? val : (val - last));
			last = val;
		};

		update_stat(GSPerfMon::Draw, GSRunner::s_total_internal_draws, GSRunner::s_last_internal_draws);
		update_stat(GSPerfMon::DrawCalls, GSRunner::s_total_draws, GSRunner::s_last_draws);
		update_stat(GSPerfMon::RenderPasses, GSRunner::s_total_render_passes, GSRunner::s_last_render_passes);
		update_stat(GSPerfMon::Barriers, GSRunner::s_total_barriers, GSRunner::s_last_barriers);
		update_stat(GSPerfMon::TextureCopies, GSRunner::s_total_copies, GSRunner::s_last_copies);
		update_stat(GSPerfMon::TextureUploads, GSRunner::s_total_uploads, GSRunner::s_last_uploads);
		update_stat(GSPerfMon::Readbacks, GSRunner::s_total_readbacks, GSRunner::s_last_readbacks);

		const bool idle_frame = GSRunner::s_total_frames && (last_draws == GSRunner::s_total_internal_draws && last_uploads == GSRunner::s_total_uploads);

		if (!idle_frame)
			GSRunner::s_total_drawn_frames++;

		GSRunner::s_total_frames++;

		std::atomic_thread_fence(std::memory_order_release);
	}
}

void Host::RequestResizeHostDisplay(s32 width, s32 height)
{
}

void Host::OnVMStarting()
{
}

void Host::OnVMStarted()
{
}

void Host::OnVMDestroyed()
{
}

void Host::OnVMPaused()
{
}

void Host::OnVMResumed()
{
}

void Host::OnGameChanged(const std::string& title, const std::string& elf_override, const std::string& disc_path,
	const std::string& disc_serial, u32 disc_crc, u32 current_crc)
{
}

void Host::OnPerformanceMetricsUpdated()
{
}

void Host::OnSaveStateLoading(const std::string_view filename)
{
}

void Host::OnSaveStateLoaded(const std::string_view filename, bool was_successful)
{
}

void Host::OnSaveStateSaved(const std::string_view filename)
{
}

void Host::RunOnCPUThread(std::function<void()> function, bool block /* = false */)
{
	pxFailRel("Not implemented");
}

void Host::RefreshGameListAsync(bool invalidate_cache)
{
}

void Host::CancelGameListRefresh()
{
}

bool Host::IsFullscreen()
{
	return false;
}

void Host::SetFullscreen(bool enabled)
{
}

void Host::OnCaptureStarted(const std::string& filename)
{
}

void Host::OnCaptureStopped()
{
}

void Host::RequestExitApplication(bool allow_confirm)
{
}

void Host::RequestExitBigPicture()
{
}

void Host::RequestVMShutdown(bool allow_confirm, bool allow_save_state, bool default_save_state)
{
	VMManager::SetState(VMState::Stopping);
}

void Host::OnAchievementsLoginSuccess(const char* username, u32 points, u32 sc_points, u32 unread_messages)
{
	// noop
}

void Host::OnAchievementsLoginRequested(Achievements::LoginRequestReason reason)
{
	// noop
}

void Host::OnAchievementsHardcoreModeChanged(bool enabled)
{
	// noop
}

void Host::OnAchievementsRefreshed()
{
	// noop
}

void Host::OnCoverDownloaderOpenRequested()
{
	// noop
}

void Host::OnCreateMemoryCardOpenRequested()
{
	// noop
}

bool Host::InBatchMode()
{
	return false;
}

bool Host::InNoGUIMode()
{
	return false;
}

bool Host::ShouldPreferHostFileSelector()
{
	return false;
}

void Host::OpenHostFileSelectorAsync(std::string_view title, bool select_directory, FileSelectorCallback callback,
	FileSelectorFilters filters, std::string_view initial_directory)
{
	callback(std::string());
}

std::optional<u32> InputManager::ConvertHostKeyboardStringToCode(const std::string_view str)
{
	return std::nullopt;
}

std::optional<std::string> InputManager::ConvertHostKeyboardCodeToString(u32 code)
{
	return std::nullopt;
}

const char* InputManager::ConvertHostKeyboardCodeToIcon(u32 code)
{
	return nullptr;
}

BEGIN_HOTKEY_LIST(g_host_hotkeys)
END_HOTKEY_LIST()

static void PrintCommandLineVersion()
{
	std::fprintf(stderr, "PCSX2 GS Runner Version %s\n", GIT_REV);
	std::fprintf(stderr, "https://pcsx2.net/\n");
	std::fprintf(stderr, "\n");
}

static void GSRunner::PrintCommandLineHelp(const char* progname)
{
	PrintCommandLineVersion();
	std::fprintf(stderr, "Usage: %s [parameters] [--] [filename]\n", progname);
	std::fprintf(stderr, "\n");
	std::fprintf(stderr, "  -help: Displays this information and exits.\n");
	std::fprintf(stderr, "  -version: Displays version information and exits.\n");
	std::fprintf(stderr, "  -dumpdir <dir>: Frame dump directory (will be dumped as filename_frameN.png).\n");
	std::fprintf(stderr, "  -dump [rt|tex|z|f|a|i|tr]: Enabling dumping of render target, texture, z buffer, frame, "
		"alphas, and info (context, vertices, transfers (list)), transfers (images), respectively, per draw. Generates lots of data.\n");
	std::fprintf(stderr, "  -dumprange N[,L,B]: Start dumping from draw N (base 0), stops after L draws, and only "
		"those draws that are multiples of B (intersection of -dumprange and -dumprangef used)."
		"Defaults to 0,-1,1 (all draws). Only used if -dump used.\n");
	std::fprintf(stderr, "  -dumprangef NF[,LF,BF]: Start dumping from frame NF (base 0), stops after LF frames, "
		"and only those frames that are multiples of BF (intersection of -dumprange and -dumprangef used).\n"
		"Defaults to 0,-1,1 (all frames). Only used if -dump is used.\n");
	std::fprintf(stderr, "  -loop <count>: Loops dump playback N times. Defaults to 1. 0 will loop infinitely.\n");
	std::fprintf(stderr, "  -renderer <renderer>: Sets the graphics renderer. Defaults to Auto.\n");
	std::fprintf(stderr, "  -swthreads <threads>: Sets the number of threads for the software renderer.\n");
	std::fprintf(stderr, "  -window: Forces a window to be displayed.\n");
	std::fprintf(stderr, "  -surfaceless: Disables showing a window.\n");
	std::fprintf(stderr, "  -logfile <filename>: Writes emu log to filename.\n");
	std::fprintf(stderr, "  -noshadercache: Disables the shader cache (useful for parallel runs).\n");
	std::fprintf(stderr, "  --: Signals that no more arguments will follow and the remaining\n"
						 "    parameters make up the filename. Use when the filename contains\n"
						 "    spaces or starts with a dash.\n");
	std::fprintf(stderr, "\n");
}

void GSTester::PrintCommandLineHelp(const char* progname)
{
	PrintCommandLineVersion();
	std::fprintf(stderr, "Usage: %s [parameters] [--] [filename]\n", progname);
	std::fprintf(stderr, "\n");
	std::fprintf(stderr, "  -help: Displays this information and exits.\n");
	std::fprintf(stderr, "\n");
}

void GSRunner::InitializeConsole()
{
	const char* var = std::getenv("PCSX2_NOCONSOLE");
	s_no_console = (var && StringUtil::FromChars<bool>(var).value_or(false));
	if (!s_no_console)
		Log::SetConsoleOutputLevel(LOGLEVEL_DEBUG);
}

bool GSRunner::ParseCommandLineArgs(int argc, char* argv[], VMBootParameters& params)
{
	std::string dumpdir; // Save from argument -dumpdir for creating sub-directories
	bool no_more_args = false;
	for (int i = 1; i < argc; i++)
	{
		if (!no_more_args)
		{
#define CHECK_ARG(str) !std::strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) (!std::strcmp(argv[i], str) && ((i + 1) < argc))

			if (i == 1 && CHECK_ARG("runner"))
			{
				continue;
			}
			else if (CHECK_ARG("-help"))
			{
				PrintCommandLineHelp(argv[0]);
				return false;
			}
			else if (CHECK_ARG("-version"))
			{
				PrintCommandLineVersion();
				return false;
			}
			else if (CHECK_ARG_PARAM("-dumpdir"))
			{
				dumpdir = s_output_prefix = StringUtil::StripWhitespace(argv[++i]);
				if (s_output_prefix.empty())
				{
					Console.Error("Invalid dump directory specified.");
					return false;
				}

				if (!FileSystem::DirectoryExists(s_output_prefix.c_str()) && !FileSystem::CreateDirectoryPath(s_output_prefix.c_str(), false))
				{
					Console.Error("Failed to create output directory");
					return false;
				}

				continue;
			}
			else if (CHECK_ARG_PARAM("-dump"))
			{
				std::string str(argv[++i]);

				s_settings_interface.SetBoolValue("EmuCore/GS", "DumpGSData", true);

				if (str.find("rt") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveRT", true);
				if (str.find("f") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveFrame", true);
				if (str.find("tex") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveTexture", true);
				if (str.find("z") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveDepth", true);
				if (str.find("a") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveAlpha", true);
				if (str.find("i") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveInfo", true);
				if (str.find("tr") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "SaveTransferImages", true);
				continue;
			}
			else if (CHECK_ARG_PARAM("-dumprange"))
			{
				std::string str(argv[++i]);

				std::vector<std::string_view> split = StringUtil::SplitString(str, ',');
				int start = 0;
				int num = -1;
				int by = 1;
				if (split.size() > 0)
				{
					start = StringUtil::FromChars<int>(split[0]).value_or(0);
				}
				if (split.size() > 1)
				{
					num = StringUtil::FromChars<int>(split[1]).value_or(-1);
				}
				if (split.size() > 2)
				{
					by = std::max(1, StringUtil::FromChars<int>(split[2]).value_or(1));
				}
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveDrawStart", start);
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveDrawCount", num);
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveDrawBy", by);
				continue;
			}
			else if (CHECK_ARG_PARAM("-dumprangef"))
			{
				std::string str(argv[++i]);

				std::vector<std::string_view> split = StringUtil::SplitString(str, ',');
				int start = 0;
				int num = -1;
				int by = 1;
				if (split.size() > 0)
				{
					start = StringUtil::FromChars<int>(split[0]).value_or(0);
				}
				if (split.size() > 1)
				{
					num = StringUtil::FromChars<int>(split[1]).value_or(-1);
				}
				if (split.size() > 2)
				{
					by = std::max(1, StringUtil::FromChars<int>(split[2]).value_or(1));
				}
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveFrameStart", start);
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveFrameCount", num);
				s_settings_interface.SetIntValue("EmuCore/GS", "SaveFrameBy", by);
				continue;
			}
			else if (CHECK_ARG_PARAM("-dumpdirhw"))
			{
				s_dump_gs_data_dir_hw = argv[++i];
				s_settings_interface.SetStringValue("EmuCore/GS", "HWDumpDirectory", argv[i]);
				continue;
			}
			else if (CHECK_ARG_PARAM("-dumpdirsw"))
			{
				s_dump_gs_data_dir_sw = argv[++i];
				s_settings_interface.SetStringValue("EmuCore/GS", "SWDumpDirectory", argv[i]);
				continue;
			}
			else if (CHECK_ARG_PARAM("-loop"))
			{
				s_loop_count = StringUtil::FromChars<s32>(argv[++i]).value_or(0);
				Console.WriteLn("Looping dump playback %d times.", s_loop_count);
				continue;
			}
			else if (CHECK_ARG_PARAM("-renderer"))
			{
				const char* rname = argv[++i];

				GSRendererType type = GSRendererType::Auto;
				if (StringUtil::Strcasecmp(rname, "Auto") == 0)
					type = GSRendererType::Auto;
#ifdef _WIN32
				else if (StringUtil::Strcasecmp(rname, "dx11") == 0)
					type = GSRendererType::DX11;
				else if (StringUtil::Strcasecmp(rname, "dx12") == 0)
					type = GSRendererType::DX12;
#endif
#ifdef ENABLE_OPENGL
				else if (StringUtil::Strcasecmp(rname, "gl") == 0)
					type = GSRendererType::OGL;
#endif
#ifdef ENABLE_VULKAN
				else if (StringUtil::Strcasecmp(rname, "vulkan") == 0)
					type = GSRendererType::VK;
#endif
#ifdef __APPLE__
				else if (StringUtil::Strcasecmp(rname, "metal") == 0)
					type = GSRendererType::Metal;
#endif
				else if (StringUtil::Strcasecmp(rname, "sw") == 0)
					type = GSRendererType::SW;
				else
				{
					Console.Error("Unknown renderer '%s'", rname);
					return false;
				}

				Console.WriteLn("Using %s renderer.", Pcsx2Config::GSOptions::GetRendererName(type));
				s_settings_interface.SetIntValue("EmuCore/GS", "Renderer", static_cast<int>(type));
				continue;
			}
			else if (CHECK_ARG_PARAM("-swthreads"))
			{
				const int swthreads = StringUtil::FromChars<int>(argv[++i]).value_or(0);
				if (swthreads < 0)
				{
					Console.WriteLn("Invalid number of software threads");
					return false;
				}
				
				Console.WriteLn(fmt::format("Setting number of software threads to {}", swthreads));
				s_settings_interface.SetIntValue("EmuCore/GS", "SWExtraThreads", swthreads);
				continue;
			}
			else if (CHECK_ARG_PARAM("-renderhacks"))
			{
				std::string str(argv[++i]);

				s_settings_interface.SetBoolValue("EmuCore/GS", "UserHacks", true);

				if (str.find("af") != std::string::npos)
					s_settings_interface.SetIntValue("EmuCore/GS", "UserHacks_AutoFlushLevel", 1);
				if (str.find("cpufb") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "UserHacks_CPU_FB_Conversion", true);
				if (str.find("dds") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "UserHacks_DisableDepthSupport", true);
				if (str.find("dpi") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "UserHacks_DisablePartialInvalidation", true);
				if (str.find("dsf") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "UserHacks_Disable_Safe_Features", true);
				if (str.find("tinrt") != std::string::npos)
					s_settings_interface.SetIntValue("EmuCore/GS", "UserHacks_TextureInsideRt", 1);
				if (str.find("plf") != std::string::npos)
					s_settings_interface.SetBoolValue("EmuCore/GS", "preload_frame_with_gs_data", true);

				continue;
			}
			else if (CHECK_ARG_PARAM("-upscale"))
			{
				const float upscale = StringUtil::FromChars<float>(argv[++i]).value_or(0.0f);
				if (upscale < 0.5f)
				{
					Console.WriteLn("Invalid upscale multiplier");
					return false;
				}

				Console.WriteLn(fmt::format("Setting upscale multiplier to {}", upscale));
				s_settings_interface.SetFloatValue("EmuCore/GS", "upscale_multiplier", upscale);
				continue;
			}
			else if (CHECK_ARG_PARAM("-logfile"))
			{
				const char* logfile = argv[++i];
				if (std::strlen(logfile) > 0)
				{
					// disable timestamps, since we want to be able to diff the logs
					Console.WriteLn("Logging to %s...", logfile);
					VMManager::Internal::SetFileLogPath(logfile);
					s_settings_interface.SetBoolValue("Logging", "EnableFileLogging", true);
					s_settings_interface.SetBoolValue("Logging", "EnableTimestamps", false);
				}

				continue;
			}
			else if (CHECK_ARG_PARAM("-regression-test"))
			{
				s_regression_file = std::string(argv[++i]);
				s_batch_mode = true;
				continue;
			}
			else if (CHECK_ARG_PARAM("-name"))
			{
				s_runner_name = argv[++i];
				continue;
			}
			else if (CHECK_ARG_PARAM("-npackets"))
			{
				s_regression_num_packets = StringUtil::FromChars<u32>(argv[++i]).value_or(GSTester::regression_num_packets_default);
				continue;
			}
			else if (CHECK_ARG_PARAM("-regression-dump-size"))
			{
				std::optional<u32> size_mb = StringUtil::FromChars<u32>(argv[++i]);
				if (size_mb.has_value())
					s_regression_dump_size = size_mb.value() * _1mb;
				continue;
			}
			else if (CHECK_ARG_PARAM("-frames-max"))
			{
				s_frames_max = StringUtil::FromChars<u32>(argv[++i]).value_or(0xFFFFFFFF);
				continue;
			}
			else if (CHECK_ARG_PARAM("-ppid"))
			{
				s_parent_pid = StringUtil::FromChars<u32>(argv[++i]).value_or(0);
				continue;
			}
			else if (CHECK_ARG_PARAM("-nbatches"))
			{
				s_num_batches = StringUtil::FromChars<u32>(argv[++i]).value_or(1);
				continue;
			}
			else if (CHECK_ARG_PARAM("-batch-id"))
			{
				s_batch_id = StringUtil::FromChars<u32>(argv[++i]).value_or(0);
				continue;
			}
			else if (CHECK_ARG("-batch"))
			{
				s_batch_mode = true;
				continue;
			}
			else if (CHECK_ARG("-noshadercache"))
			{
				Console.WriteLn("Disabling shader cache");
				s_settings_interface.SetBoolValue("EmuCore/GS", "disable_shader_cache", true);
				continue;
			}
			else if (CHECK_ARG("-window"))
			{
				Console.WriteLn("Creating window");
				s_use_window = true;
				continue;
			}
			else if (CHECK_ARG("-surfaceless"))
			{
				Console.WriteLn("Running surfaceless");
				s_use_window = false;
				continue;
			}
			else if (CHECK_ARG("--"))
			{
				no_more_args = true;
				continue;
			}
			else if (argv[i][0] == '-')
			{
				Console.Error("Unknown parameter: '%s'", argv[i]);
				return false;
			}

#undef CHECK_ARG
#undef CHECK_ARG_PARAM
		}

		if (!params.filename.empty())
			params.filename += ' ';
		params.filename += argv[i];
	}

	if (s_runner_name.empty())
	{
		s_runner_name = std::filesystem::path(argv[0]).filename().string();
	}

	if (!s_regression_file.empty())
		return true; // Remaining arguments/checks are not needed if doing regression testing.

	if (params.filename.empty())
	{
		Console.Error("No dump filename provided and not in regression testing mode..");
		return false;
	}

	if (s_batch_mode)
	{
		if (!FileSystem::DirectoryExists(params.filename.c_str()))
		{
			Console.Error("Provided directory does not exist.");
			return false;
		}

		s_num_batches = std::max(s_num_batches, 1u);
		s_batch_id = s_batch_id % s_num_batches;
	}
	else
	{
		if (!VMManager::IsGSDumpFileName(params.filename))
		{
			Console.Error("Provided filename is not a GS dump.");
			return false;
		}
	}

	if (s_settings_interface.GetBoolValue("EmuCore/GS", "DumpGSData") && !dumpdir.empty())
	{
		s_dump_gs_data_dir_hw = dumpdir;
		s_dump_gs_data_dir_sw = dumpdir;
		if (s_settings_interface.GetStringValue("EmuCore/GS", "HWDumpDirectory").empty())
			s_settings_interface.SetStringValue("EmuCore/GS", "HWDumpDirectory", dumpdir.c_str());
		if (s_settings_interface.GetStringValue("EmuCore/GS", "SWDumpDirectory").empty())
			s_settings_interface.SetStringValue("EmuCore/GS", "SWDumpDirectory", dumpdir.c_str());
		
		// Disable saving frames with SaveSnapshotToMemory()
		// Instead we save more "raw" snapshots when using -dump.
		s_output_prefix = "";
	}

	// set up the frame dump directory
	if (!s_output_prefix.empty())
	{
		if (!s_batch_mode)
		{
			// strip off all extensions
			std::string_view title(Path::GetFileTitle(params.filename));
			if (StringUtil::EndsWithNoCase(title, ".gs"))
				title = Path::GetFileTitle(title);

			s_output_prefix = Path::Combine(s_output_prefix, StringUtil::StripWhitespace(title));
			Console.WriteLn(fmt::format("Saving dumps as {}_frameN.png", s_output_prefix));
		}
		else
		{
			Console.WriteLn(fmt::format("Saving dumps to {}", s_output_prefix));
		}
	}

	return true;
}

bool GSTester::ParseCommandLineArgs(int argc, char* argv[], u32 thread_id)
{
	for (int i = 1; i < argc; i++)
	{
#define CHECK_ARG(str) !std::strcmp(argv[i], str)
#define ENSURE_ARG_COUNT(str, n) \
	do { \
		if (i + n >= argc) \
		{ \
			Console.Error("Not enough arguments for " str " (need " #n ")"); \
			return false; \
		} \
	} while (0)
		
		if (i == 1 && CHECK_ARG("tester"))
		{
			continue;
		}
		else if (CHECK_ARG("-help"))
		{
			PrintCommandLineHelp(argv[0]);
			return false;
		}
		else if (CHECK_ARG("-version"))
		{
			PrintCommandLineVersion();
			return false;
		}
		else if (CHECK_ARG("-input"))
		{
			ENSURE_ARG_COUNT("-input", 1);

			regression_dump_dir = StringUtil::StripWhitespace(argv[++i]);
			if (regression_dump_dir.empty())
			{
				Console.Error("Invalid input directory/file specified.");
				return false;
			}

			if (!FileSystem::DirectoryExists(regression_dump_dir.c_str()) && !FileSystem::FileExists(regression_dump_dir.c_str()))
			{
				Console.Error("Input directory/file does not exist.");
				return false;
			}

			continue;
		}
		else if (CHECK_ARG("-output"))
		{
			ENSURE_ARG_COUNT("-output", 1);

			regression_output_dir = StringUtil::StripWhitespace(argv[++i]);
			if (regression_output_dir.empty())
			{
				Console.Error("Invalid output directory specified.");
				return false;
			}

			Error e;
			if (!FileSystem::EnsureDirectoryExists(regression_output_dir.c_str(), true, &e))
			{
				Console.ErrorFmt("Error creating/checking directory: {}", e.GetDescription());
				return false;
			}

			continue;
		}
		else if (CHECK_ARG("-path"))
		{
			ENSURE_ARG_COUNT("-path", 2);

			regression_runner_path[0] = StringUtil::StripWhitespace(std::string(argv[++i]));
			regression_runner_path[1] = StringUtil::StripWhitespace(std::string(argv[++i]));

			continue;
		}
		else if (CHECK_ARG("-name"))
		{
			ENSURE_ARG_COUNT("-name", 2);

			regression_runner_name[0] = StringUtil::StripWhitespace(std::string(argv[++i]));
			regression_runner_name[1] = StringUtil::StripWhitespace(std::string(argv[++i]));

			continue;
		}
		else if (CHECK_ARG("-npackets"))
		{
			ENSURE_ARG_COUNT("-npackets", 2);

			regression_num_packets = StringUtil::FromChars<u32>(argv[++i]).value_or(regression_num_packets_default);

			continue;
		}
		else if (CHECK_ARG("-regression-dump-size"))
		{
			ENSURE_ARG_COUNT("-regression-dump-size", 1);

			std::optional<u32> size_mb = StringUtil::FromChars<u32>(argv[++i]);
			if (size_mb.has_value())
				GSTester::regression_dump_size = size_mb.value() * _1mb;

			continue;
		}
		else if (CHECK_ARG("-verbose-level"))
		{
			ENSURE_ARG_COUNT("-verbose-level", 1);

			regression_verbose_level = StringUtil::FromChars<u32>(argv[++i]).value_or(regression_verbose_level_default);

			continue;
		}
		else if (CHECK_ARG("-nthreads"))
		{
			i++; // Skip -- handled by the main thread.
			continue;
		}
		else
		{
			regression_runner_args.append(argv[i]);
			regression_runner_args.append(" ");
			continue;
		}

#undef CHECK_ARG
	}

	if (regression_dump_dir.empty())
	{
		Console.Error("Dump directory/file not provided.");
		return false;
	}

	if (regression_output_dir.empty())
	{
		Console.Error("Output directory not provided.");
		return false;
	}

		for (int i = 0; i < 2; i++)
	{
		if (regression_runner_path[i].empty())
		{
			Console.ErrorFmt("Runner {} path not provided.", i + 1);
			return false;
		}

		if (!FileSystem::FileExists(regression_runner_path[i].c_str()))
		{
			Console.ErrorFmt("Runner {} path does not exist: \"{}\"", i + 1, regression_runner_path[i]);
			return false;
		}

		if (regression_runner_name[i].empty())
		{
			regression_runner_name[i] = std::filesystem::path(regression_runner_path[i]).filename().string();
		}
	}

	if (regression_runner_name[0] == regression_runner_name[1])
	{
		// Need unique names for output directories.
		regression_runner_name[0] += " (1)";
		regression_runner_name[1] += " (2)";
	}

	for (int i = 0; i < 2; i++)
	{
		regression_output_image_dir[i] = (std::filesystem::path(regression_output_dir) / "image" / regression_runner_name[i]).string();
		regression_output_hwstat_dir[i] = (std::filesystem::path(regression_output_dir) / "hwstat" / regression_runner_name[i]).string();

		Error e;
		if (!FileSystem::EnsureDirectoryExists(regression_output_image_dir[i].c_str(), true, &e))
		{
			Console.ErrorFmt("Unable to create output directory '{}' (error: {})", regression_output_image_dir[i], e.GetDescription());
			return false;
		}
		if (!FileSystem::EnsureDirectoryExists(regression_output_hwstat_dir[i].c_str(), true, &e))
		{
			Console.ErrorFmt("Unable to create output directory '{}' (error: {})", regression_output_hwstat_dir[i], e.GetDescription());
			return false;
		}
	}

	return true;
}

void GSRunner::DumpStats()
{
	std::atomic_thread_fence(std::memory_order_acquire);
	Console.WriteLn(fmt::format("======= HW STATISTICS FOR {} ({}) FRAMES ========", s_total_frames, s_total_drawn_frames));
	Console.WriteLn(fmt::format("@HWSTAT@ Draw Calls: {} (avg {})", s_total_draws, static_cast<u64>(std::ceil(s_total_draws / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn(fmt::format("@HWSTAT@ Render Passes: {} (avg {})", s_total_render_passes, static_cast<u64>(std::ceil(s_total_render_passes / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn(fmt::format("@HWSTAT@ Barriers: {} (avg {})", s_total_barriers, static_cast<u64>(std::ceil(s_total_barriers / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn(fmt::format("@HWSTAT@ Copies: {} (avg {})", s_total_copies, static_cast<u64>(std::ceil(s_total_copies / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn(fmt::format("@HWSTAT@ Uploads: {} (avg {})", s_total_uploads, static_cast<u64>(std::ceil(s_total_uploads / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn(fmt::format("@HWSTAT@ Readbacks: {} (avg {})", s_total_readbacks, static_cast<u64>(std::ceil(s_total_readbacks / static_cast<double>(s_total_drawn_frames)))));
	Console.WriteLn("============================================");
}

#ifdef _WIN32
// We can't handle unicode in filenames if we don't use wmain on Win32.
#define main real_main
#endif

static void CPUThreadMain(VMBootParameters* params)
{
	if (VMManager::Initialize(*params))
	{
		// run until end
		GSDumpReplayer::SetLoopCount(GSRunner::s_loop_count);
		Host::PumpMessagesOnCPUThread(); // Make sure we have the correct loop count/frame.
		VMManager::SetState(VMState::Running);
		while (VMManager::GetState() == VMState::Running)
			VMManager::Execute();
		VMManager::Shutdown(false);
		if (!GSRunner::s_batch_mode)
			GSRunner::DumpStats();
	}

	VMManager::Internal::CPUThreadShutdown();
	GSRunner::StopPlatformMessagePump();
}

GSTester::ReturnValue GSTester::CopyDumpToSharedMemory(const std::unique_ptr<GSDumpFile>& dump, const std::string& name)
{
	Error error;
	GSDumpFileSharedMemory* dump_shared[2]{};

	std::size_t size;

	for (int i = 0; i < 2; i++)
	{
		dump_shared[i] = regression_buffer[i].GetDumpWrite(false); // Don't block
		if (!dump_shared[i])
		{
			return BUFFER_NOT_READY;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			if (!dump->ReadFile(dump_shared[0]->GetPtrDump(), regression_dump_size, &size, &error))
			{
				Console.ErrorFmt("(GSTester) Failed to read GS dump from memory (error: {}).", error.GetDescription());
				return ERROR_;
			}
		}
		else
		{
			memcpy(dump_shared[1]->GetPtrDump(), dump_shared[0]->GetPtrDump(), size);
		}

		dump_shared[i]->SetSizeDump(size);
		dump_shared[i]->SetNameDump(name);
	}

	for (int i = 0; i < 2; i++)
		regression_buffer[i].DoneDumpWrite(); // Only commit on successful write; otherwise leave empty.

	return SUCCESS;
}

GSTester::ReturnValue GSTester::ProcessPackets()
{
	Error error;
	GSRegressionPacket* packets[2];

	ScopedGuard done_read([&]() {
		if (packets[0] && packets[1])
		{
			for (int i = 0; i < 2; i++)
			{
				regression_buffer[i].DonePacketRead();
			}
		}
	});

	for (int i = 0; i < 2; i++)
	{
		packets[i] = regression_buffer[i].GetPacketRead(false);
	}

	// Have a packets from both runners. Compare and output if different.
	if (packets[0] && packets[1])
	{
		std::string name_dump[2];
		std::string name_packet[2];
		u32 type_packet[2];

		for (int i = 0; i < 2; i++)
		{
			name_dump[i] = packets[i]->GetNameDump();
			name_packet[i] = packets[i]->GetNamePacket();
			type_packet[i] = packets[i]->type;
		}

		if (name_dump[0] == name_dump[1])
		{
			DumpInfo* dump_curr = &regression_dumps[regression_dumps_map.at(name_dump[0])];

			if (name_packet[0] == name_packet[1] && type_packet[0] == type_packet[1])
			{
				const std::string& packet_name_curr = name_packet[0];
				u32 packet_type_curr = type_packet[0];

				if (regression_verbose_level >= VERBOSE_TESTER)
					Console.WriteLnFmt("(GSTester) Comparing results for {} / {}.", dump_curr->name, packet_name_curr);

				if (packet_type_curr == GSRegressionPacket::IMAGE)
				{
					if (RegressionCompareImages(packets[0], packets[1], 0) != 0.0f)
					{
						for (int i = 0; i < 2; i++)
						{
							std::string image_dir = (std::filesystem::path(regression_output_image_dir[i]) / dump_curr->name).string();

							if (!FileSystem::EnsureDirectoryExists(image_dir.c_str(), true, &error))
							{
								Console.ErrorFmt("(GSTester) Unable to create directory: '{}' (error: {})", image_dir, error.GetDescription());
								dump_curr->packets_skipped++;
								return ERROR_;
							}

							std::string image_file = (std::filesystem::path(image_dir) / packet_name_curr).string();

							if (!GSPng::Save(GSPng::RGB_A_PNG, image_file, packets[i]->image.data, packets[i]->w, packets[i]->h, packets[i]->pitch, GSConfig.PNGCompressionLevel, false))
							{
								Console.ErrorFmt("(GSTester) Unable to save image file: '{}'", image_file);
								dump_curr->packets_skipped++;
								return ERROR_;
							}
						}
					}
				}
				else if (packet_type_curr == GSRegressionPacket::HWSTAT)
				{
					if (packets[0]->hwstat != packets[1]->hwstat)
					{
						for (int i = 0; i < 2; i++)
						{
							std::string hwstat_file = (std::filesystem::path(regression_output_hwstat_dir[i]) / (packet_name_curr + ".txt")).string();

							std::ofstream oss(hwstat_file);

							if (!oss.is_open())
							{
								Console.ErrorFmt("(GSTester) Unable to open HW stat file: '{}'", hwstat_file);
								dump_curr->packets_skipped++;
								return ERROR_;
							}

							oss << "frames" << KEY_VAL_DEL << packets[i]->hwstat.frames << std::endl;
							oss << "draws" << KEY_VAL_DEL << packets[i]->hwstat.draws << std::endl;
							oss << "render_passes" << KEY_VAL_DEL << packets[i]->hwstat.render_passes << std::endl;
							oss << "barriers" << KEY_VAL_DEL << packets[i]->hwstat.barriers << std::endl;
							oss << "copies" << KEY_VAL_DEL << packets[i]->hwstat.copies << std::endl;
							oss << "uploads" << KEY_VAL_DEL << packets[i]->hwstat.uploads << std::endl;
							oss << "readbacks" << KEY_VAL_DEL << packets[i]->hwstat.readbacks << std::endl;

							oss.close();
						}
					}
				}
				else if (packet_type_curr == GSRegressionPacket::DONE_DUMP)
				{
					Console.WriteLnFmt("(GSTester) Completed dump '{}'", dump_curr->name);
					regression_dump_last_completed = dump_curr->name;
					dump_curr->state = DumpInfo::COMPLETED;
					dump_curr->packets_completed++;
				}
				else
				{
					Console.ErrorFmt("(GSTester) Unknown packet type '{}'", packet_type_curr);
					dump_curr->packets_skipped++;
					return ERROR_;
				}

				dump_curr->packets_completed++; // Packet processed successfully.
				return SUCCESS;
			}
			else
			{
				Console.Error("(GSTester) Runners out of sync on following dumps:");
				for (int i = 0; i < 2; i++)
				{
					Console.ErrorFmt("    {}: {} / {}", regression_runner_name[i], name_dump[i], name_packet[i]);
				}
				dump_curr->packets_skipped++;
				return ERROR_;
			}
		}
		else
		{
			Console.Error("(GSTester) Runners out of sync on following dumps:");
			for (int i = 0; i < 2; i++)
			{
				Console.ErrorFmt("    {}: {} / {}", regression_runner_name[i], name_dump[i], name_packet[i]);
			}
			regression_packets_skipped_unknown++;
			return ERROR_;
		}
	}
	else
	{
		return BUFFER_NOT_READY;
	}
}

int GSRunner::main_runner(int argc, char* argv[])
{
	if (!InitializeConfig())
	{
		Console.Error("Failed to initialize config.");
		return EXIT_FAILURE;
	}

	VMBootParameters params;
	if (!ParseCommandLineArgs(argc, argv, params))
		return EXIT_FAILURE;

	if (!VMManager::Internal::CPUThreadInitialize())
		return EXIT_FAILURE;

	if (s_use_window.value_or(true) && !CreatePlatformWindow())
	{
		Console.Error("Failed to create window.");
		return EXIT_FAILURE;
	}

	// Regression testing needs to be started before applying settings
	// or it might complain that there is no dumping directory
	// (regression test data is dumped to memory).
	if (!s_regression_file.empty())
	{
		GSStartRegressionTest(&s_regression_buffer, s_regression_file,
			s_regression_num_packets, s_regression_dump_size);

		if (s_parent_pid == 0)
		{
			Console.ErrorFmt("Regression testing without a valid parent PID.");
			return EXIT_FAILURE;
		}

		if (!GSProcess::SetParentPID(s_parent_pid))
		{
			Console.ErrorFmt("Unable to open parent PID {}.", s_parent_pid);
			return EXIT_FAILURE;
		}

		Console.WriteLnFmt("(GSRunner/{}) Opened parent PID {}.", s_runner_name, s_parent_pid);
	}
	
	// apply new settings (e.g. pick up renderer change)
	VMManager::ApplySettings();
	GSDumpReplayer::SetIsDumpRunner(true, s_runner_name);
	GSDumpReplayer::SetFrameNumberMax(s_frames_max);
	if (s_batch_mode)
	{
		GSDumpReplayer::SetIsBatchMode(true);
		GSDumpReplayer::SetNumBatches(s_num_batches);
		GSDumpReplayer::SetBatchID(s_batch_id);
		GSDumpReplayer::SetLoopCountStart(s_loop_count);
		if (s_settings_interface.GetBoolValue("EmuCore/GS", "DumpGSData", false))
		{
			GSDumpReplayer::SetDumpGSDataDirHW(s_dump_gs_data_dir_hw);
			GSDumpReplayer::SetDumpGSDataDirSW(s_dump_gs_data_dir_sw);
		}
	}

	std::thread cputhread(CPUThreadMain, &params);
	PumpPlatformMessages(/*forever=*/true);
	cputhread.join();

	VMManager::Internal::CPUThreadShutdown();
	DestroyPlatformWindow();
	if (GSIsRegressionTesting())
		GSEndRegressionTest();
	return EXIT_SUCCESS;
}

bool GSTester::DumpCached::Load(DumpInfo& d, std::size_t max_dump_size, Error* error)
{
	s64 size = FileSystem::GetPathFileSize(d.file.c_str());
	if (size < 0)
	{
		Error::SetStringFmt(error, "Unable to stat file '{}'", d.file);
		return false;
	}

	// Check the disk size before trying to decompress to fail quickly.
	// The decompressed size is usually more than 4x the disk size.
	if (static_cast<std::size_t>(4 * size) > max_dump_size)
	{
		Error::SetStringFmt(error, "Disk size too large '{}' ({} bytes)", d.file, size);
		return false;
	}

	ptr = GSDumpFile::OpenGSDump(d.file.c_str(), error);
	if (!ptr)
		return false;
	name = d.name;
	return true;
}

bool GSTester::DumpCached::HasCached()
{
	return static_cast<bool>(ptr);
}

void GSTester::DumpCached::Reset()
{
	ptr.reset();
	name = "";
}

bool GSTester::StartRunners()
{
	const auto quote = [](std::string x) { return "\"" + x + "\""; };

	// Start the runner processes in regression testing mode.
	for (int i = 0; i < 2; i++)
	{
		GSProcess::PID_t pid = GSProcess::GetCurrentPID();

		if (regression_runner_command[i].empty())
		{
			regression_runner_command[i] =
				quote(regression_runner_path[i]) +
				std::string(" runner ") +
				std::string(" -surfaceless ") +
				std::string(" -loop 1 ") +
				std::string(" -dump f ") +
				std::string(" -regression-test ") + quote(regression_shared_file[i]) +
				std::string(" -name ") + quote(regression_runner_name[i]) +
				std::string(" -npackets ") + std::to_string(regression_num_packets) +
				std::string(" -regression-dump-size ") + std::to_string(regression_dump_size / _1mb) +
				std::string(" -ppid ") + std::to_string(pid) +
				" " + regression_runner_args;
		}

		if (!regression_runner_proc[i].Start(regression_runner_command[i], regression_verbose_level < VERBOSE_TESTER_AND_RUNNER))
		{
			Console.ErrorFmt("(GSTester) Unable to start runner: {} (command: {})", regression_runner_name[i],
				regression_runner_command[i]);
			return false;
		}

		Console.WriteLnFmt("(GSTester) Created runner process (PID: {}) with command: '{}'",
			regression_runner_proc[i].GetPID(), regression_runner_command[i]);
	}

	// Wait until the runners initialize and hit the dump waiting loop.
	Common::Timer timer;
	constexpr int timeout = 10;
	while (true)
	{
		if (timer.GetTimeSeconds() > static_cast<double>(timeout))
		{
			Console.ErrorFmt("(GSTester) Both runners not initialized after {} seconds.", timeout);
			return false;
		}

		if (regression_buffer[0].GetStateRunner() == GSRegressionBuffer::WAIT_DUMP &&
			regression_buffer[1].GetStateRunner() == GSRegressionBuffer::WAIT_DUMP)
		{
			return true;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));

		Console.WriteLn("(GSTester) Waiting for runner processes...");
	}

	return true; // Unreachable
}

// Try to end the runner processes gracefully and otherwise terminate them.
bool GSTester::EndRunners()
{
	for (int i = 0; i < 2; i++)
		regression_buffer[i].SetStateTester(GSRegressionBuffer::EXIT);

	constexpr double terminate_timeout = 20.0; // Seconds to wait before forcefully terminating processes.
	Common::Timer timer;
	double sec;
	while (1)
	{
		if (!regression_runner_proc[0].IsRunning() && !regression_runner_proc[1].IsRunning())
		{
			break;
		}

		if ((sec = timer.GetTimeSeconds()) >= terminate_timeout)
			break;

		std::this_thread::sleep_for(std::chrono::seconds(1));

		Console.WriteLnFmt("(GSTester) Waiting for runners to exit...");
	}

	if (regression_runner_proc[0].IsRunning() || regression_runner_proc[1].IsRunning())
	{
		Console.ErrorFmt("(GSTester) Unable to safely end runner processes...terminating.");
		for (int i = 0; i < 2; i++)
			regression_runner_proc[i].Terminate();
	}

	return !regression_runner_proc[0].IsRunning() && !regression_runner_proc[1].IsRunning();
}

bool GSTester::RestartRunners()
{
	if (!EndRunners())
		return false;

	for (int i = 0; i < 2; i++)
		regression_buffer[i].Reset();

	return StartRunners();
}

bool GSTester::GetDumpInfo(u32 nthreads, u32 thread_id)
{
	std::vector<std::string> dump_files;

	if (VMManager::IsGSDumpFileName(regression_dump_dir))
	{
		dump_files.push_back(regression_dump_dir);
	}
	else if (FileSystem::DirectoryExists(regression_dump_dir.c_str()))
	{
		GSDumpReplayer::GetDumpFileList(regression_dump_dir, dump_files, nthreads, thread_id);
	}
	else
	{
		Console.WarningFmt("(GSTester) Provided file is neither a dump or a directory: '{}'", regression_dump_dir);
		return false;
	}

	for (const std::string& file : dump_files)
	{
		std::string name = std::filesystem::path(file).filename().string();
		regression_dumps.push_back(DumpInfo(file, name));
	}

	for (std::size_t i = 0; i < regression_dumps.size(); i++)
	{
		regression_dumps_map.insert({regression_dumps[i].name, i});
	}

	return true;
}

int GSTester::MainThread(int argc, char* argv[], u32 nthreads, u32 thread_id)
{
	if (!ParseCommandLineArgs(argc, argv, thread_id))
		return EXIT_FAILURE;

	if (!GetDumpInfo(nthreads, thread_id))
		return EXIT_FAILURE;

	Console.WriteLnFmt("(GSTester) Found {} dumps in '{}'", regression_dumps.size(), regression_dump_dir);

	Console.WriteLn("(GSTester) Opening shared memory files.");
	for (int i = 0; i < 2; i++)
	{
		regression_shared_file[i] = "regression-test-file-" + std::to_string(GetCurrentProcessId()) +
			"-" + std::to_string(thread_id) + "-" + std::to_string(i);

		if (!regression_buffer[i].CreateFile_(regression_shared_file[i], regression_num_packets, regression_dump_size))
		{
			Console.ErrorFmt("(GSTester) Unable to create regression shared file: {}", regression_shared_file[i]);
			return EXIT_FAILURE;
		}

		Console.WriteLnFmt("(GSTester) Created regression packets file: {}", regression_shared_file[i]);
	}

	Console.WriteLn("(GSTester) Starting runner processes.");
	if (!StartRunners())
	{
		Console.Error("(GSTester) Unable to start runner processes. Exiting.");
		return EXIT_FAILURE;
	}
	Console.WriteLn("(GSTester) Runners processes are initialized.");

	std::size_t dump_index = 0; // Current dump that should be written to dump buffer.
	constexpr double deadlock_timeout = 10.0; // Seconds before determining a deadlock in runner.
	Common::Timer deadlock_timer; // Time since seeing a packet from a runner.
	constexpr std::size_t max_failure_restarts = 10; // Max times to attempt restarting before giving up.
	std::size_t failure_restarts = 0; // Current number of failure restarts.

	// Temporary loop variables.
	ReturnValue retval;
	DumpCached dump; // Cache the dump from disk to shared with runner processes.
	Error error; // Current error.
	bool fail = false; // Signals a failure aat some point in processing.

	Console.WriteLn("(GSTester) Starting main testing loop.");

	// Main testing loop.
	while (1)
	{
		if (fail)
		{
			fail = false;

			if (++regression_failure_restarts >= regression_failure_restarts_max)
			{
				Console.ErrorFmt("(GSTester) Attempted restarting {} times due to failures...exiting.", regression_failure_restarts);
				EndRunners();
				break;
			}
			else
			{
				Console.ErrorFmt("(GSTester) Attempting to restart due to failure (attempt {}).", regression_failure_restarts);

				// Reset dump to the last one we got packets for.
				if (regression_dump_last_completed.empty())
				{
					Console.ErrorFmt("(GSTester) No dumps completed; starting from beginning.");
					dump_index = 0;
				}
				else
				{
					Console.ErrorFmt("(GSTester) Restarting from {}.", regression_dump_last_completed);
					dump_index = regression_dumps_map.at(regression_dump_last_completed) + 1;
				}

				// Reset stats of subsequent dumps.
				for (std::size_t i = dump_index; i < regression_dumps.size(); i++)
				{
					regression_dumps[i].packets_skipped = 0;
					regression_dumps[i].packets_completed = 0;
					regression_dumps[i].state = DumpInfo::UNVISITED;
				}

				// Restart the runner processes
				if (!RestartRunners())
				{
					Console.ErrorFmt("(GSTester) Failed to restart.");
					break;
				}
			}
		}

		if (dump_index < regression_dumps.size())
		{
			if (dump.HasCached())
			{
				retval = CopyDumpToSharedMemory(dump.ptr, dump.name);

				if (retval == SUCCESS)
				{
					Console.WriteLnFmt("(GSTester) Copied '{}' to shared memory", dump.name);
					dump.Reset();
					dump_index++;
					deadlock_timer.Reset(); // Decompressing the dump can take time. Don't want false positives.
				}
				else if (retval == ERROR_)
				{
					Console.ErrorFmt("(GSTester) Error copying '{}' to shared memory.", regression_dumps[dump_index].file);
					dump.Reset();
					regression_dumps[dump_index].state = DumpInfo::SKIPPED;
					dump_index++;
				}
				else // retval == BUFFER_NOT_READY
				{
					// Try again next iteration.
				}
			}
			else if (!dump.Load(regression_dumps[dump_index], regression_dump_size, &error))
			{
				Console.ErrorFmt("(GSTester) Failed to load dump '{}' (error: {}).", regression_dumps[dump_index].file,
					error.GetDescription());
				regression_dumps[dump_index].state = DumpInfo::SKIPPED;
				dump_index++;
			}

			if (dump_index >= regression_dumps.size())
			{
				Console.WriteLn("(GSTester) Done uploading dumps.");
				for (int i = 0; i < 2; i++)
					regression_buffer[i].SetStateTester(GSRegressionBuffer::DONE_UPLOADING);
			}
		}

		retval = ProcessPackets();

		if (retval == SUCCESS)
		{
			deadlock_timer.Reset();
		}
		else if (retval == ERROR_)
		{
			fail = true;
			continue;
		}
		else // retval == BUFFER_NOT_READY
		{
			// Try again next iteration.
		}

		// See if all dumps are completed or remaining dumps are all skipped.
		std::size_t i = regression_dump_last_completed.empty() ?
			0 :
			regression_dumps_map.at(regression_dump_last_completed) + 1;
		while (i < regression_dumps.size() && regression_dumps[i].state == DumpInfo::SKIPPED)
		{
			i++;
		}
		if (i >= regression_dumps.size())
		{
			Console.WriteLn("(GSTester) All dumps/packets finished.");
			break;
		}

		// Handle possible deadlock.
		if (deadlock_timer.GetTimeSeconds() >= deadlock_timeout)
		{
			if (regression_dump_last_completed.empty())
				Console.ErrorFmt("(GSTester) Possible deadlock on dump {}.", regression_dumps[0].name);
			else
				Console.ErrorFmt("(GSTester) Possible deadlock detected after dump {}.", regression_dump_last_completed);
			deadlock_timer.Reset();
			fail = true;
			continue;
		}

		std::this_thread::yield();
	}

	EndRunners();

	for (int i = 0; i < 2; i++)
	{
		if (regression_runner_proc[i].WaitForExit() != 0)
		{
			Console.WarningFmt("(GSTester) Runner {} exited abnormally.", regression_runner_name[i]);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		regression_runner_proc[i].Close();
	}

	for (int i = 0; i < 2; i++)
		regression_buffer[i].CloseFile();

	// Get stats for main thread.
	regression_dumps_completed = 0;
	regression_dumps_skipped = 0;
	regression_packets_completed = 0;
	regression_packets_skipped = 0;
	regression_dumps_unvisited = 0;
	for (std::size_t i = 0; i < regression_dumps.size(); i++)
	{
		regression_packets_completed += regression_dumps[i].packets_completed;
		regression_packets_skipped += regression_dumps[i].packets_skipped;
		regression_dumps_completed += regression_dumps[i].state == DumpInfo::COMPLETED;
		regression_dumps_skipped += regression_dumps[i].state == DumpInfo::SKIPPED;
		regression_dumps_unvisited += regression_dumps[i].state == DumpInfo::UNVISITED;
	}

	return EXIT_SUCCESS;
}

int GSTester::main_tester(int argc, char* argv[])
{
	Common::Timer timer_total;

	int nthreads = 1;
	for (int i = 1; i < argc; i++)
	{
		if (strncmp(argv[i], "-nthreads", 9) == 0)
		{
			if (i + 1 >= argc)
			{
				Console.ErrorFmt("(GSTester) Expected an argument for '-nthreads'");
				return EXIT_FAILURE;
			}
			nthreads = StringUtil::FromChars<u32>(argv[++i]).value_or(1);
		}
	}

	nthreads = std::clamp(nthreads, 1, 8);

	Console.WriteLnFmt("(GSTester) Running regression test with {} threads.", nthreads);

	std::vector<GSTester> testers;
	std::vector<int> return_value;
	std::vector<std::thread> threads;
	
	testers.resize(nthreads);
	return_value.resize(nthreads);
	threads.resize(nthreads);

	for (int i = 0; i < nthreads; i++)
		return_value[i] = EXIT_FAILURE;

	const auto run_thread = [&](u32 thread_id) {
		return_value[thread_id] = testers[thread_id].MainThread(argc, argv, nthreads, thread_id);
	};
	
	for (int i = 1; i < nthreads; i++)
	{
		threads[i] = std::thread(run_thread, i);
	}

	// Run ID 0 on main thread and rest on worker threads.
	run_thread(0);
	for (int i = 1; i < nthreads; i++)
		threads[i].join();

	std::string threads_failed;
	for (int i = 0; i < nthreads; i++)
	{
		if (return_value[i] != EXIT_SUCCESS)
		{
			if (!threads_failed.empty())
				threads_failed += ", ";
			threads_failed += std::to_string(i);
		}
	}

	if (threads_failed.empty())
	{
		Console.WriteLn("(GSTester) All threads succeeded.");
	}
	else
	{
		Console.WriteLnFmt("(GSTester) The follow threads failed: {}", threads_failed);
	}

	std::size_t dumps_total;
	std::size_t dumps_completed = 0;
	std::size_t dumps_skipped = 0;
	std::size_t dumps_unvisited = 0;
	std::size_t packets_completed = 0;
	std::size_t packets_skipped = 0;
	std::size_t packets_skipped_unknown = 0;
	std::size_t failure_restarts = 0;

	for (int i = 0; i < nthreads; i++)
	{
		dumps_completed += testers[i].regression_dumps_completed;
		dumps_skipped += testers[i].regression_dumps_skipped;
		dumps_unvisited += testers[i].regression_dumps_unvisited;
		packets_completed += testers[i].regression_packets_completed;
		packets_skipped += testers[i].regression_packets_skipped;
		packets_skipped_unknown += testers[i].regression_packets_skipped_unknown;
		failure_restarts += testers[i].regression_failure_restarts;
	}

	dumps_total = dumps_completed + dumps_skipped + dumps_unvisited;

	Console.WriteLnFmt("GSTester Stats:");
	Console.WriteLnFmt("    Run time: {:.2} minutes", timer_total.GetTimeSeconds() / 60.0);
	Console.WriteLnFmt("    Dumps completed: {} / {}", dumps_completed, dumps_total);
	Console.WriteLnFmt("    Dumps skipped: {} / {}", dumps_skipped, dumps_total);
	Console.WriteLnFmt("    Dumps unvisited: {} / {}", dumps_unvisited, dumps_total);
	Console.WriteLnFmt("    Packets completed: {} (Avg {})",
		packets_completed, packets_completed / dumps_total);
	Console.WriteLnFmt("    Packets skipped: {} (Avg {})",
		packets_completed, packets_completed / dumps_total);
	Console.WriteLnFmt("    Packets skipped unknown: {} (Avg {})",
		packets_skipped_unknown, packets_skipped_unknown / dumps_total);
	Console.WriteLnFmt("    Failure restarts: {}", failure_restarts);

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	CrashHandler::Install();
	GSRunner::InitializeConsole();

	if (argc < 2)
	{
		Console.Error("Need at least one argument");
		return EXIT_FAILURE;
	}

	std::string mode(argv[1]);

	if (mode == "tester")
	{
		return GSTester::main_tester(argc, argv);
	}
	else
	{
		return GSRunner::main_runner(argc, argv);
	}
}

void Host::PumpMessagesOnCPUThread()
{
	// update GS thread copy of frame number
	MTGS::RunOnGSThread([frame_number = GSDumpReplayer::GetFrameNumber()]() { GSRunner::s_dump_frame_number = frame_number; });
	MTGS::RunOnGSThread([loop_number = GSDumpReplayer::GetLoopCount()]() { GSRunner::s_loop_number = loop_number; });
}

s32 Host::Internal::GetTranslatedStringImpl(
	const std::string_view context, const std::string_view msg, char* tbuf, size_t tbuf_space)
{
	if (msg.size() > tbuf_space)
		return -1;
	else if (msg.empty())
		return 0;

	std::memcpy(tbuf, msg.data(), msg.size());
	return static_cast<s32>(msg.size());
}

std::string Host::TranslatePluralToString(const char* context, const char* msg, const char* disambiguation, int count)
{
	TinyString count_str = TinyString::from_format("{}", count);

	std::string ret(msg);
	for (;;)
	{
		std::string::size_type pos = ret.find("%n");
		if (pos == std::string::npos)
			break;

		ret.replace(pos, pos + 2, count_str.view());
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////
// Platform specific code
//////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

static constexpr LPCWSTR WINDOW_CLASS_NAME = L"PCSX2GSRunner";
static HWND s_hwnd = NULL;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool GSRunner::CreatePlatformWindow()
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hIconSm = NULL;

	if (!RegisterClassExW(&wc))
	{
		Console.Error("Window registration failed.");
		return false;
	}

	s_hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, WINDOW_CLASS_NAME, L"PCSX2 GS Runner",
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH,
		WINDOW_HEIGHT, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
	if (!s_hwnd)
	{
		Console.Error("CreateWindowEx failed.");
		return false;
	}

	ShowWindow(s_hwnd, SW_SHOW);
	UpdateWindow(s_hwnd);

	// make sure all messages are processed before returning
	PumpPlatformMessages();
	return true;
}

void GSRunner::DestroyPlatformWindow()
{
	if (!s_hwnd)
		return;

	PumpPlatformMessages();
	DestroyWindow(s_hwnd);
	s_hwnd = {};
}

std::optional<WindowInfo> GSRunner::GetPlatformWindowInfo()
{
	WindowInfo wi;

	if (s_hwnd)
	{
		RECT rc = {};
		GetWindowRect(s_hwnd, &rc);
		wi.surface_width = static_cast<u32>(rc.right - rc.left);
		wi.surface_height = static_cast<u32>(rc.bottom - rc.top);
		wi.surface_scale = 1.0f;
		wi.type = WindowInfo::Type::Win32;
		wi.window_handle = s_hwnd;
	}
	else
	{
		wi.type = WindowInfo::Type::Surfaceless;
	}

	return wi;
}

static constexpr int SHUTDOWN_MSG = WM_APP + 0x100;
static DWORD MainThreadID;

void GSRunner::PumpPlatformMessages(bool forever)
{
	MSG msg;
	while (true)
	{
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == SHUTDOWN_MSG)
				return;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		if (!forever)
			return;
		WaitMessage();
	}
}

void GSRunner::StopPlatformMessagePump()
{
	PostThreadMessageW(MainThreadID, SHUTDOWN_MSG, 0, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int wmain(int argc, wchar_t** argv)
{
	std::vector<std::string> u8_args;
	u8_args.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; i++)
		u8_args.push_back(StringUtil::WideStringToUTF8String(argv[i]));

	std::vector<char*> u8_argptrs;
	u8_argptrs.reserve(u8_args.size());
	for (int i = 0; i < argc; i++)
		u8_argptrs.push_back(u8_args[i].data());
	u8_argptrs.push_back(nullptr);

	MainThreadID = GetCurrentThreadId();

	return real_main(argc, u8_argptrs.data());
}

#elif defined(__APPLE__)

static void* s_window;
static WindowInfo s_wi;

bool GSRunner::CreatePlatformWindow()
{
	pxAssertRel(!s_window, "Tried to create window when there already was one!");
	s_window = CocoaTools::CreateWindow("PCSX2 GS Runner", WINDOW_WIDTH, WINDOW_HEIGHT);
	CocoaTools::GetWindowInfoFromWindow(&s_wi, s_window);
	PumpPlatformMessages();
	return s_window;
}

void GSRunner::DestroyPlatformWindow()
{
	if (s_window) {
		CocoaTools::DestroyWindow(s_window);
		s_window = nullptr;
	}
}

std::optional<WindowInfo> GSRunner::GetPlatformWindowInfo()
{
	WindowInfo wi;
	if (s_window)
		wi = s_wi;
	else
		wi.type = WindowInfo::Type::Surfaceless;
	return wi;
}

void GSRunner::PumpPlatformMessages(bool forever)
{
	CocoaTools::RunCocoaEventLoop(forever);
}

void GSRunner::StopPlatformMessagePump()
{
	CocoaTools::StopMainThreadEventLoop();
}

#endif // _WIN32 / __APPLE__

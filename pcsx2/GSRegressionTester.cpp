#include "GSRegressionTester.h"
#include "common/Assertions.h"
#include "common/Console.h"
#include "common/ScopedGuard.h"
#include "common/Timer.h"
#include "common/Path.h"
#include "common/StringUtil.h"

#include <thread>
#include <atomic>

static GSRegressionBuffer* regression_buffer; // Used by GS runner processes.

__forceinline static void CopyStringToBuffer(char* dst, std::size_t dst_size, const std::string& src)
{
	if (src.length() > dst_size - 1)
	{
		Console.Warning("String too long for buffer.");
	}

	std::size_t n = std::min(src.length(), dst_size - 1);
	std::memcpy(dst, src.c_str(), n);
	dst[n] = '\0';
}

GSIntSharedMemory::ValType GSIntSharedMemory::CompareExchange(ValType expected, ValType desired)
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, desired, expected);
#else
	val.compare_exchange_strong(expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst);
	return expected;
#endif
}

GSIntSharedMemory::ValType GSIntSharedMemory::Get()
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, 0, 0);
#else
	return val.load(std::memory_order_seq_cst);
#endif
}

void GSIntSharedMemory::Set(ValType i)
{
#ifdef __WIN32__
	InterlockedExchange(&val, i);
#else
	val.store(i, std::memory_order_seq_cst);
#endif
}

GSIntSharedMemory::ValType GSIntSharedMemory::FetchAdd()
{
#ifdef __WIN32__
	return InterlockedIncrement(&val) - 1;
#else
	return val::fetch_add(1, std::memory_order_seq_cst) - 1;
#endif
}

void GSIntSharedMemory::Init(bool reset)
{
	// Need correct alignment.
	pxAssertRel(reinterpret_cast<size_t>(&val) % sizeof(ValType) == 0, "Atomic not aligned correctly");
#ifndef __WIN32__
	if (!reset)
		new (&val) std::atomic<ValType>;
	pxAssert(std::atomic_is_lock_free(&val), "Atomic must be lock free.");
#endif
	Set(0);
}

std::size_t GSIntSharedMemory::GetTotalSize()
{
	return sizeof(GSIntSharedMemory);
}

void GSSpinlockSharedMemory::Init(bool reset)
{
	lock.Init(reset);
	lock.Set(WRITEABLE);
}

bool GSSpinlockSharedMemory::LockWrite(GSEvent* event, std::function<bool()> cond)
{
	while (true)
	{
		if (cond && cond())
			return false;

		if (lock.CompareExchange(WRITEABLE, WRITEABLE) == WRITEABLE) // Check the lock
			return true; // Acquired write.

		if (!cond) // Fail after 1 try when non-blocking.
			return false;

		if (event)
			event->Wait(GSRegressionBuffer::EVENT_WAIT_SECONDS);
		else
			std::this_thread::yield();
	}
}

bool GSSpinlockSharedMemory::LockRead(GSEvent* event, std::function<bool()> cond)
{
	while (true)
	{
		if (cond && cond())
			return false;

		if (lock.CompareExchange(READABLE, READABLE) == READABLE) // Check the lock
			return true; // Acquired read.

		if (!cond) // Fail after 1 try when non-blocking.
			return false;

		if (event)
			event->Wait(GSRegressionBuffer::EVENT_WAIT_SECONDS);
		else
			std::this_thread::yield();
	}
}

bool GSSpinlockSharedMemory::UnlockWrite()
{
	bool success = lock.CompareExchange(WRITEABLE, READABLE) == WRITEABLE;
	if (!success)
		pxFail("Trying to unlock write when not writeable.");
	return success;
}


bool GSSpinlockSharedMemory::UnlockRead()
{
	bool success = lock.CompareExchange(READABLE, WRITEABLE) == READABLE;
	if (!success)
		pxFail("Trying to unlock read when not readable.");
	return success;
}

bool GSSpinlockSharedMemory::Writeable()
{
	return lock.Get() == WRITEABLE;
}

bool GSSpinlockSharedMemory::Readable()
{
	return lock.Get() == READABLE;
}

bool GSSpinlockSharedMemory::Lock(GSEvent* event, std::function<bool()> cond)
{
	while (true)
	{
		if (cond && cond())
			return false;

		if (lock.CompareExchange(LOCKED, UNLOCKED) == UNLOCKED) // Check the lock
			return true; // Locked successfully.

		if (!cond)
			return false; // Fail after 1 try when non-blocking.

		if (event)
			event->Wait(GSRegressionBuffer::EVENT_WAIT_SECONDS);
		else
			std::this_thread::yield();
	}
}

bool GSSpinlockSharedMemory::Unlock()
{
	bool success = lock.CompareExchange(UNLOCKED, LOCKED) == LOCKED;
	if (!success)
		pxFail("Trying to unlock when not locked.");
	return success;
}

GSRegressionPacket* GSRegressionBuffer::GetPacketWrite(std::function<bool()> cond)
{
	event[RUNNER].Reset();

	if (!GetPacket(packet_write % num_packets)->lock.LockWrite(&event[RUNNER], cond))
		return nullptr;

	return GetPacket(packet_write % num_packets);
}

GSRegressionPacket* GSRegressionBuffer::GetPacketRead(std::function<bool()> cond)
{
	event[TESTER].Reset();

	if (!GetPacket(packet_read % num_packets)->lock.LockRead(&event[TESTER], cond))
		return nullptr;

	return GetPacket(packet_read % num_packets);
}

void GSRegressionBuffer::DonePacketWrite()
{
	if (!GetPacket(packet_write % num_packets)->lock.UnlockWrite())
		pxFail("Unlock packet write is broken.");

	event[TESTER].Signal();

	packet_write++;
}

void GSRegressionBuffer::DonePacketRead()
{
	if (!GetPacket(packet_read % num_packets)->lock.UnlockRead())
		pxFail("Unlock packet read is broken.");

	event[RUNNER].Signal();

	packet_read++;
}

GSDumpFileSharedMemory* GSRegressionBuffer::GetDumpWrite(std::function<bool()> cond)
{
	event[TESTER].Reset();

	if (!GetDump(dump_write % num_dumps)->lock.LockWrite(&event[TESTER], cond))
		return nullptr;

	return GetDump(dump_write % num_dumps);
}

GSDumpFileSharedMemory* GSRegressionBuffer::GetDumpRead(std::function<bool()> cond)
{
	event[RUNNER].Reset();

	if (!GetDump(dump_read % num_dumps)->lock.LockRead(&event[RUNNER], cond))
		return nullptr;

	return GetDump(dump_read % num_dumps);
}

void GSRegressionBuffer::DoneDumpWrite()
{
	if (!GetDump(dump_write % num_dumps)->lock.UnlockWrite())
		pxFail("Unlock dump write is broken.");

	event[RUNNER].Signal();

	dump_write++;
}

void GSRegressionBuffer::DoneDumpRead()
{
	if (!GetDump(dump_read % num_dumps)->lock.UnlockRead())
		pxFail("Unlock dump read is broken.");

	event[TESTER].Signal();

	dump_read++;
}

std::size_t GSDumpFileSharedMemory::GetTotalSize(std::size_t dump_size)
{
	return sizeof(GSDumpFileSharedMemory) + dump_size;
}

void GSRegressionPacket::SetNamePacket(const std::string& path)
{
	SetName(name_packet, path);
}

void GSRegressionPacket::SetNameDump(const std::string& path)
{
	SetName(name_dump, path);
}

void GSRegressionPacket::SetName(char* dst, const std::string& path)
{
	std::string name(Path::GetFileName(path));

	CopyStringToBuffer(dst, name_size, path);
}

void GSRegressionPacket::SetImage(const void* src, int w, int h, int pitch, int bytes_per_pixel)
{
	if (src)
	{
		std::size_t src_size = h * pitch;

		if (src_size > packet_size)
		{
			Console.WarningFmt(
				"Image data is too large for regression packet (w={}, h={}, pitch={}, bytes_per_pixel={}, buffer size={}).",
				w, h, pitch, bytes_per_pixel, packet_size);
			
			h = std::min(static_cast<int>(packet_size / pitch), h);

			src_size = h * pitch;

			Console.WarningFmt(
				"Cropping image to (w={}, h={}, pitch={}, bytes_per_pixel={}, buffer size={}).",
				w, h, pitch, bytes_per_pixel, packet_size);

			pxAssertRel(src_size <= packet_size, "Cropped image still too large (impossible).");
		}

		std::memcpy(GetData(), src, src_size);

	}

	this->type = IMAGE;
	this->image_header.w = w;
	this->image_header.h = h;
	this->image_header.pitch = pitch;
	this->image_header.bytes_per_pixel = bytes_per_pixel;
}

void GSRegressionPacket::SetHWStat(const HWStat& hwstat)
{
	this->type = HWSTAT;
	this->hwstat = hwstat;
}

void GSRegressionPacket::SetDoneDump()
{
	this->type = DONE_DUMP;
}

void GSRegressionPacket::Init(std::size_t packet_size, bool reset)
{
	std::memset(GetData(), 0, packet_size);
	lock.Init(reset);
	this->type = NONE;
	std::memset(name_dump, 0, sizeof(name_dump));
	std::memset(name_packet, 0, sizeof(name_packet));
	this->packet_size = packet_size;
};

std::size_t GSRegressionPacket::GetTotalSize(std::size_t packet_size)
{
	return sizeof(GSRegressionPacket) + packet_size;
}

std::string GSRegressionPacket::GetNameDump()
{
	name_dump[std::size(name_dump) - 1] = '\0';

	return std::string(name_dump);
}

std::string GSRegressionPacket::GetNamePacket()
{
	name_packet[std::size(name_packet) - 1] = '\0';

	return std::string(name_packet);
}

void* GSRegressionPacket::GetData()
{
	return reinterpret_cast<u8*>(this) + sizeof(GSRegressionPacket);
}

const void* GSRegressionPacket::GetData() const
{
	return reinterpret_cast<const u8*>(this) + sizeof(GSRegressionPacket);
}

// Only once before sharing. Not thread safe.
void GSDumpFileSharedMemory::Init(std::size_t dump_size, bool reset)
{
	lock.Init(reset);
	std::memset(name, 0, sizeof(name));
	this->dump_size = dump_size;
	std::memset(GetPtrDump(), 0, dump_size);
}

static std::size_t GetTotalSize(std::size_t dump_size)
{
	return sizeof(GSDumpFileSharedMemory) + dump_size;
}

void* GSDumpFileSharedMemory::GetPtrDump()
{
	return reinterpret_cast<u8*>(this) + sizeof(GSDumpFileSharedMemory);
}

std::size_t GSDumpFileSharedMemory::GetSizeDump()
{
	return dump_size;
}

void GSDumpFileSharedMemory::SetSizeDump(std::size_t size)
{
	dump_size = size;
}

std::string GSDumpFileSharedMemory::GetNameDump()
{
	name[std::size(name) - 1] = '\0';

	return std::string(name);
}

void GSDumpFileSharedMemory::SetNameDump(const std::string& str)
{
	CopyStringToBuffer(name, name_size, str);
}

std::size_t GSRegressionBuffer::GetTotalSize(std::size_t num_packets, std::size_t packet_size, std::size_t num_dumps, std::size_t dump_size)
{
	return num_packets * GSRegressionPacket::GetTotalSize(packet_size) +
		num_dumps * GSDumpFileSharedMemory::GetTotalSize(dump_size) +
		num_states * GSIntSharedMemory::GetTotalSize();
}

void GSRegressionBuffer::SetSizesPointers(std::size_t num_packets, std::size_t packet_size, std::size_t num_dumps, std::size_t dump_size)
{
	std::size_t packet_offset;
	std::size_t dump_file_offset;
	std::size_t state_offset;

	const std::size_t start_offset = reinterpret_cast<std::size_t>(shm.data);
	std::size_t curr_offset = start_offset;

	packet_offset = curr_offset;
	curr_offset += num_packets * GSRegressionPacket::GetTotalSize(packet_size);

	dump_file_offset = curr_offset;
	curr_offset += num_dumps * GSDumpFileSharedMemory::GetTotalSize(dump_size);

	state_offset = curr_offset;
	curr_offset += num_states * GSIntSharedMemory::GetTotalSize();

	pxAssert(curr_offset - start_offset == GetTotalSize(num_packets, packet_size, num_dumps, dump_size));

	packets = reinterpret_cast<GSRegressionPacket*>(packet_offset);
	dumps = reinterpret_cast<GSDumpFileSharedMemory*>(dump_file_offset);
	state = reinterpret_cast<GSIntSharedMemory*>(state_offset);

	this->num_packets = num_packets;
	this->packet_size = packet_size;
	this->num_dumps = num_dumps;
	this->dump_size = dump_size;
}

bool GSRegressionBuffer::CreateFile_(
	const std::string& name,
	const std::string& event_runner_name,
	const std::string& event_tester_name,
	std::size_t num_packets,
	std::size_t packet_size,
	std::size_t num_dumps,
	std::size_t dump_size)
{
	if (!shm.CreateFile_(name, GetTotalSize(num_packets, packet_size, num_dumps, dump_size)))
		return false;

	if (!event[RUNNER].Create(event_runner_name))
	{
		CloseFile();
		return false;
	}

	// Should be created by tester process before.
	if (!event[TESTER].Open_(event_tester_name))
	{
		CloseFile();
		return false;
	}

	SetSizesPointers(num_packets, packet_size, num_dumps, dump_size);

	Init(name, num_packets, packet_size, num_dumps, dump_size);

	Reset();

	return true;
}

void GSRegressionBuffer::Init(
	const std::string& name,
	std::size_t num_packets,
	std::size_t packet_size,
	std::size_t num_dumps,
	std::size_t dump_size)
{
	for (std::size_t i = 0; i < num_packets; i++)
		GetPacket(i)->Init(packet_size);
	for (std::size_t i = 0; i < num_dumps; i++)
		GetDump(i)->Init(dump_size);
	for (std::size_t i = 0; i < num_states; i++)
		state[i].Init();
	for (std::size_t i = 0; i < num_events; i++)
		event[i].Reset();
}

void GSRegressionBuffer::Reset()
{
	this->packet_write = 0;
	this->packet_read = 0;
	this->dump_write = 0;
	this->dump_read = 0;
	this->dump_name.clear();

	for (std::size_t i = 0; i < num_packets; i++)
		GetPacket(i)->Init(packet_size, true);
	for (std::size_t i = 0; i < num_dumps; i++)
		GetDump(i)->Init(dump_size, true);
	for (std::size_t i = 0; i < num_states; i++)
		state[i].Set(DEFAULT);
	for (std::size_t i = 0; i < num_events; i++)
		event[i].Reset();
}

bool GSRegressionBuffer::OpenFile(
	const std::string& name,
	const std::string& event_runner_name,
	const std::string& event_tester_name,
	std::size_t num_packets,
	std::size_t packet_size,
	std::size_t num_dumps,
	std::size_t dump_size)
{
	if (!shm.OpenFile(name, GetTotalSize(num_packets, packet_size, num_dumps, dump_size)))
		return false;

	if (!event[RUNNER].Open_(event_runner_name))
	{
		CloseFile();
		return false;
	}

	if (!event[TESTER].Open_(event_tester_name))
	{
		CloseFile();
		return false;
	}

	SetSizesPointers(num_packets, packet_size, num_dumps, dump_size);

	return true;
}

bool GSRegressionBuffer::CloseFile()
{
	packets = nullptr;
	dumps = nullptr;
	state = nullptr;
	num_packets = 0;
	num_dumps = 0;

	for (std::size_t i = 0; i < num_events; i++)
		event[i].Close();

	if (!shm.CloseFile())
		return false;

	return true;
}

void GSRegressionBuffer::SetState(u32 which, u32 s)
{
	state[which].Set(static_cast<GSIntSharedMemory::ValType>(s));
}

void GSRegressionBuffer::SetStateRunner(u32 s)
{
	SetState(RUNNER, s);
}

void GSRegressionBuffer::SetStateTester(u32 s)
{
	SetState(TESTER, s);
}

u32 GSRegressionBuffer::GetState(u32 which)
{
	return static_cast<u32>(state[which].Get());
};

u32 GSRegressionBuffer::GetStateRunner()
{
	return GetState(RUNNER);
};

u32 GSRegressionBuffer::GetStateTester()
{
	return GetState(TESTER);
};

void GSRegressionBuffer::SetNameDump(const std::string& name)
{
	dump_name = name;
}

std::string GSRegressionBuffer::GetNameDump()
{
	return dump_name;
}

void GSRegressionBuffer::SignalRunnerHeartbeat()
{
	state[RUNNER_HEARTBEAT].Set(ALIVE);
}

bool GSRegressionBuffer::CheckRunnerHeartbeat()
{
	return state[RUNNER_HEARTBEAT].Get() == ALIVE;
}

void GSRegressionBuffer::ResetRunnerHeartbeat()
{
	state[RUNNER_HEARTBEAT].Set(DEFAULT);
}

GSRegressionPacket* GSRegressionBuffer::GetPacket(std::size_t i)
{
	pxAssertMsg(i < num_packets, "Index out of bounds.");
	return reinterpret_cast<GSRegressionPacket*>(reinterpret_cast<u8*>(packets) + i * GSRegressionPacket::GetTotalSize(packet_size));
}

GSDumpFileSharedMemory* GSRegressionBuffer::GetDump(std::size_t i)
{
	pxAssertMsg(i < num_dumps, "Index out of bounds.");
	return reinterpret_cast<GSDumpFileSharedMemory*>(reinterpret_cast<u8*>(dumps) + i * GSDumpFileSharedMemory::GetTotalSize(dump_size));
}

void GSRegressionBuffer::DebugDumpBuffer()
{
	Console.WarningFmt("Dump buffer debug (write={}, read={}):", dump_write, dump_read);
	for (std::size_t i = 0; i < num_dumps; i++)
	{
		GSDumpFileSharedMemory* dump = GetDump(i);
		std::size_t name_size = strnlen(dump->name, std::size(dump->name));
		std::string name(dump->name, name_size);
		Console.WarningFmt("    {}: lock={} name='{}' size={}", i, GSSpinlockSharedMemory::STATE_STR_WR.at(dump->lock.lock.Get()), name, dump->dump_size);
	}
}

void GSRegressionBuffer::DebugPacketBuffer()
{
	Console.WarningFmt("Packet buffer debug (write={}, read={}):", packet_write, packet_read);
	for (std::size_t i = 0; i < num_packets; i++)
	{
		GSRegressionPacket* packet = GetPacket(i);

		std::size_t name_packet_size = strnlen(packet->name_packet, std::size(packet->name_packet));
		std::size_t name_dump_size = strnlen(packet->name_dump, std::size(packet->name_dump));

		std::string name_packet(packet->name_packet, name_packet_size);
		std::string name_dump(packet->name_dump, name_dump_size);

		Console.WarningFmt("    {}: lock={} name_dump='{}' name_packet='{}'", i, GSSpinlockSharedMemory::STATE_STR_WR.at(packet->lock.lock.Get()), name_dump, name_packet);
	}
}

void GSRegressionBuffer::DebugState()
{
	Console.WarningFmt("runner_state={} tester_state={}", STATE_STR.at(state[RUNNER].Get()), STATE_STR.at(state[TESTER].Get()));
}

bool GSIsRegressionTesting()
{
	return regression_buffer != nullptr;
}

/// Start regression testing within the producer/GS runner process.
void GSStartRegressionTest(
	GSRegressionBuffer* rbp,
	const std::string& fn,
	const std::string& event_name_runner,
	const std::string& event_name_tester,
	std::size_t num_packets,
	std::size_t packet_size,
	std::size_t num_dumps,
	std::size_t dump_size)
{
	if (!rbp->OpenFile(
		fn,
		event_name_runner,
		event_name_tester,
		num_packets,
		packet_size,
		num_dumps,
		dump_size))
	{
		pxFail("Unable to start regression test.");
		return;
	}

	Console.WriteLnFmt("Opened {} for regression testing.", fn);

	regression_buffer = rbp;
}

void GSEndRegressionTest()
{
	if (!regression_buffer)
	{
		pxFail("No regression buffer to close.");
		return;
	}

	if (!regression_buffer->CloseFile())
	{
		pxFail("Unable to end regression test.");
		return;
	}
}

GSRegressionBuffer* GSGetRegressionBuffer()
{
	return regression_buffer;
}

void GSSignalRunnerHeartbeat()
{
	if (regression_buffer)
		regression_buffer->SignalRunnerHeartbeat();
	else
		Console.ErrorFmt("Not regression testing.");
}

int GSRegressionImageMemCmp(const GSRegressionPacket* p1, const GSRegressionPacket* p2)
{
	const GSRegressionPacket::ImageHeader& img1 = p1->image_header;
	const GSRegressionPacket::ImageHeader& img2 = p2->image_header;

	if (img1.w != img2.w || img1.h != img2.h || img1.bytes_per_pixel != img2.bytes_per_pixel)
		return INT_MAX; // Formats are different.

	int w = img1.w;
	int h = img2.h;
	int bytes_per_pixel = img1.bytes_per_pixel;
	return StringUtil::StrideMemCmp(p1->GetData(), img1.pitch, p2->GetData(), img2.pitch, w * bytes_per_pixel, h);
}

bool GSCheckTesterStatus(bool exit, bool done_uploading)
{
	if (regression_buffer)
	{
		u32 state = regression_buffer->GetStateTester();
		return
			(exit && state == GSRegressionBuffer::EXIT) ||
			(exit && !GSProcess::IsParentRunning()) ||
			(done_uploading && state == GSRegressionBuffer::DONE_UPLOADING);
	}
	else
	{
		Console.ErrorFmt("Not regression testing.");
		return false;
	}
}

bool GSProcess::Start(const std::string& command, bool detached)
{
#ifdef __WIN32__
	std::memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	std::memset(&pi, 0, sizeof(PROCESS_INFORMATION));

	std::wstring wcommand = StringUtil::UTF8StringToWideString(command);
	std::vector<wchar_t> wcommand_buf(wcommand.begin(), wcommand.end());
	wcommand_buf.push_back(L'\0');

	HANDLE hNull = INVALID_HANDLE_VALUE; // For redirecting child's stdout/err.

	ScopedGuard close_null([&]() {
		if (hNull != INVALID_HANDLE_VALUE)
			CloseHandle(hNull);
	});

	if (detached)
	{
		// Redirect stdout/err/in to null.

		hNull = CreateFileA(
			"NUL",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hNull == INVALID_HANDLE_VALUE)
		{
			Console.Error("Unable to open null handle");
			return false;
		}

		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = hNull;
		si.hStdOutput = hNull;
		si.hStdInput = hNull;
	}

	if (!CreateProcess(
			NULL,
			wcommand_buf.data(),
			NULL,
			NULL,
			TRUE,
			(DWORD)0,
			NULL,
			NULL,
			&si,
			&pi))
	{
		Console.ErrorFmt("Unable to create runner process with command: '{}'", command);
		return false;
	}

		
	if (detached)
	{
		// Redirect stdout/err to null.

		HANDLE null = INVALID_HANDLE_VALUE;

		ScopedGuard close_null([&]() {
			if (null != INVALID_HANDLE_VALUE)
				CloseHandle(null);
		});

		null = CreateFileA(
			"NUL",
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (null == INVALID_HANDLE_VALUE)
		{
			Console.Error("Unable to open null handle");
			return false;
		}
	}

	this->command = command;

	return true;
#else
	// Not implemented.
	return false;
#endif
}

bool GSProcess::IsRunning(Handle_t handle, double seconds)
{
#ifdef __WIN32__
	DWORD status = WaitForSingleObject(handle, static_cast<DWORD>(seconds * 1000.0));

	if (status == WAIT_FAILED)
	{
		Console.ErrorFmt("Waiting for process {} failed.", handle);
		return false;
	}

	return status == WAIT_TIMEOUT;
#else
	return false; // Not implemented
#endif
}

bool GSProcess::IsRunning(double seconds)
{
#ifdef __WIN32__
	if (!pi.hProcess)
	{
		Console.ErrorFmt("Do not have a valid handle.");
		return false;
	}
	return IsRunning(pi.hProcess, seconds);
#else
	// Not implemented
	return false;
#endif
}

bool GSProcess::WaitForExit(double seconds)
{
#ifdef __WIN32__
	return IsRunning(pi.hProcess, seconds);
#else
	// Not implemented
	return false;
#endif
}

bool GSProcess::Close()
{
#ifdef __WIN32__
	return CloseHandle(pi.hProcess) && CloseHandle(pi.hThread);
#else
	// Not implemented
	return false;
#endif
}

void GSProcess::Terminate()
{
#ifdef __WIN32__
	TerminateProcess(pi.hProcess, EXIT_FAILURE);
#else
	// Not implemented
#endif
}

GSProcess::PID_t GSProcess::GetPID()
{
#ifdef __WIN32__
	return pi.dwProcessId;
#else
	return 0; // Not implemented.
#endif
}

GSProcess::PID_t GSProcess::current_pid = 0;
GSProcess::PID_t GSProcess::parent_pid = 0;
GSProcess::Handle_t GSProcess::parent_h = 0;

bool GSProcess::SetParentPID(PID_t pid)
{
#ifdef __WIN32__
	parent_pid = pid;
	parent_h = OpenProcess(SYNCHRONIZE, FALSE, pid);
	return parent_h != 0;
#else
	return false; // Not implemented
#endif
}

GSProcess::PID_t GSProcess::GetParentPID()
{
	return parent_pid;
}

GSProcess::PID_t GSProcess::GetCurrentPID()
{
	if (current_pid == 0)
	{
#ifdef __WIN32__
		current_pid = GetCurrentProcessId();
#else
		// Not implemented.
#endif
	}
	return current_pid;
}

bool GSProcess::IsParentRunning(double seconds)
{
	if (!parent_pid || !parent_h)
	{
		Console.ErrorFmt("Do not have a valid parent PID and/or handle.");
		return false;
	}
	return IsRunning(parent_h, seconds);
}

// Windows defines CreateFile as a macro so use CreateFile_.
bool GSSharedMemoryFile::CreateFile_(const std::string& name, std::size_t size)
{
#ifdef __WIN32__
	handle = CreateFileMappingA(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		size,
		name.c_str());

	if (!handle)
	{
		Console.ErrorFmt("Failed to create regression packets file: {}", name);
		return false;
	}

	data = static_cast<GSRegressionPacket*>(
		MapViewOfFile(
			handle,
			FILE_MAP_WRITE,
			0,
			0,
			size));

	if (!data)
	{
		Console.ErrorFmt("Failed to map view of regressions packet file: {}", name);
		CloseHandle(handle);
		return false;
	}

	this->name = name;
	this->size = size;

	return true;
#else
	// Not implemented.
#endif
}

bool GSSharedMemoryFile::OpenFile(const std::string& name, std::size_t size)
{
	// Note: num_packets must match the value used in creation!
#ifdef __WIN32__
	handle = OpenFileMappingA(FILE_MAP_WRITE, FALSE, name.c_str());
	if (!handle)
	{
		Console.ErrorFmt("Not able to open file for regression packets: {}", name);
		return false;
	}

	data = static_cast<GSRegressionPacket*>(MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, size));
	if (!data)
	{
		Console.ErrorFmt("Unable to map regression packet file to memory: {}", name);
		CloseHandle(handle);
		return false;
	}

	Console.WriteLnFmt("Opened/mapped regression packet file: {}", name);

	this->name = name;
	this->size = size;

	return true;
#else
	return false; // Not implemented.
#endif
}

bool GSSharedMemoryFile::CloseFile()
{
#ifdef __WIN32__
	if (!handle)
	{
		Console.Error("There is no handle to close.");
		return false;
	}

	if (!CloseHandle(handle))
	{
		Console.Error("Failed to close file handle.");
		return false;
	}

	if (!data)
	{
		Console.Error("There is no file view to unmap.");
		return false;
	}

	if (!UnmapViewOfFile(data))
	{
		Console.Error("Failed to unmap file view.");
		return false;
	}

	Console.WriteLnFmt("Closed/unmapped shared memory file: {}", name);

	name = "";
	handle = 0;
	data = nullptr;

	return true;
#else
	return false; // Not implemented.
#endif
}

void GSSharedMemoryFile::ResetFile()
{
	std::memset(data, 0, size);
}

#ifdef __WIN32__
std::wstring GSEvent::GetGlobalName(const std::string& name)
{
	return StringUtil::UTF8StringToWideString("Global\\" + name);
}
#endif

bool GSEvent::Create(const std::string& name)
{
#ifdef __WIN32__
	std::wstring name_global = GetGlobalName(name);

	handle = CreateEvent(
		nullptr,
		TRUE,
		FALSE,
		name_global.c_str());

	if (!handle)
	{
		Console.ErrorFmt("Failed to create event: {} (error: {}).", name, GetLastError());
		return false;
	}

	this->name = name;

	return true;
#else
	return false; // Not implemented.
#endif
}

std::string GSEvent::GetName()
{
#ifdef __WIN32__
	return name;
#else
	return ""; // Not implemented.
#endif
}

bool GSEvent::Open_(const std::string& name)
{
#ifdef __WIN32__
	std::wstring name_global = GetGlobalName(name);

	handle = OpenEvent(
		SYNCHRONIZE | EVENT_MODIFY_STATE,
		FALSE,
		name_global.c_str());

	if (!handle)
	{
		Console.ErrorFmt("Failed to open event: {} (error: {}).", name, GetLastError());
		return false;
	}

	this->name = name;

	return true;
#else
	return false;
#endif
}

bool GSEvent::Close() const
{
#ifdef __WIN32__
	return CloseHandle(handle);
#else
	return false; // Not implemented.
#endif
}

bool GSEvent::Signal() const
{
#ifdef __WIN32__
	if (!SetEvent(handle))
	{
		Console.ErrorFmt("Set event {} failed (Windows error: {}).", name, GetLastError());
		return false;
	}

	return true;
#else
	return false; // Not implemented
#endif
}

bool GSEvent::Wait(double seconds) const
{
#ifdef __WIN32__
	DWORD status = WaitForSingleObject(handle, static_cast<DWORD>(seconds * 1000.0));

	if (status == WAIT_FAILED)
	{
		Console.ErrorFmt("Waiting for event {} failed (Windows error: {}).", name, GetLastError());
		return false;
	}

	return status != WAIT_TIMEOUT;
#else
	return false; // Not implemented.
#endif
}

bool GSEvent::Reset() const
{
#ifdef __WIN32__
	if (!ResetEvent(handle))
	{
		Console.ErrorFmt("Set event {} failed (Windows error: {}).", name, GetLastError());
		return false;
	}

	return true;
#else
	return false; // Not implemented.
#endif
}

void GSStringQueueIPC::SetSizesPointers(std::size_t num_strings, std::size_t num_runners)
{
	std::size_t head_offset;
	std::size_t file_status_offset;
	std::size_t strings_offset;
	std::size_t state_parent_offset;
	std::size_t state_child_offset;
	std::size_t runner_heartbeats_offset;

	std::size_t start_offset = reinterpret_cast<std::size_t>(shm.data);
	std::size_t curr_offset = start_offset;

	head_offset = curr_offset;
	curr_offset += sizeof(GSIntSharedMemory);

	file_status_offset = curr_offset;
	curr_offset += num_strings * sizeof(GSIntSharedMemory);

	strings_offset = curr_offset;
	curr_offset += num_strings * string_size;

	state_parent_offset = curr_offset;
	curr_offset += num_runners * sizeof(GSIntSharedMemory);

	state_child_offset = curr_offset;
	curr_offset += num_runners * sizeof(GSIntSharedMemory);

	runner_heartbeats_offset = curr_offset;
	curr_offset += num_runners * sizeof(GSIntSharedMemory);

	pxAssert(curr_offset - start_offset == GetTotalSize(num_strings, num_runners));

	head = reinterpret_cast<GSIntSharedMemory*>(head_offset);
	file_status = reinterpret_cast<GSIntSharedMemory*>(file_status_offset);
	strings = reinterpret_cast<char*>(strings_offset);
	state_parent = reinterpret_cast<GSIntSharedMemory*>(state_parent_offset);
	state_child = reinterpret_cast<GSIntSharedMemory*>(state_child_offset);
	runner_heartbeats = reinterpret_cast<GSIntSharedMemory*>(runner_heartbeats_offset);

	this->num_strings = num_strings;
	this->num_runners = num_runners;
}

bool GSStringQueueIPC::CreateFile_(const std::string& name, std::size_t num_strings, std::size_t num_runners)
{
	if (!shm.CreateFile_(name, GetTotalSize(num_strings, num_runners)))
		return false;

	SetSizesPointers(num_strings, num_runners);

	Init();
	return true;
}

bool GSStringQueueIPC::OpenFile_(const std::string& name, std::size_t num_strings, std::size_t num_runners)
{
	if (!shm.OpenFile(name, GetTotalSize(num_strings, num_runners)))
		return false;

	SetSizesPointers(num_strings, num_runners);
	return true;
}

void GSStringQueueIPC::Init()
{
	head->Init();

	for (std::size_t i = 0; i < num_strings; i++)
	{
		file_status[i].Init();
	}

	for (std::size_t i = 0; i < num_runners; i++)
	{
		state_parent[i].Init();
		state_child[i].Init();
		runner_heartbeats[i].Init();
	}
}

bool GSStringQueueIPC::AcquireString(std::string& string)
{
	std::size_t i = head->FetchAdd();

	if (i >= num_strings)
	{
		head->Set(i);
		string.clear();
		return false;
	}
	else
	{
		string = GetString(i);
		return true;
	}
}

bool GSStringQueueIPC::CheckRunnerIndex(std::size_t i)
{
	if (i >= num_runners)
	{
		Console.ErrorFmt("GSStringQueueIPC: Runner index out of bounds ({} / {}).", i, num_runners);
		pxFail("");
		return false;
	}

	return true;
}

bool GSStringQueueIPC::CheckFileIndex(std::size_t i)
{
	if (i >= num_strings)
	{
		Console.ErrorFmt("GSStringQueueIPC: File index out of bounds ({} / {}).", i, num_strings);
		pxFail("");
		return false;
	}

	return true;
}

GSStringQueueIPC::FileStatus GSStringQueueIPC::GetFileStatus(std::size_t i)
{
	if (!CheckFileIndex(i))
		return ERROR_FS;

	return static_cast<FileStatus>(file_status[i].Get());
}

void GSStringQueueIPC::SetFileStatus(std::size_t i, FileStatus status)
{
	if (!CheckFileIndex(i))
		return;

	file_status[i].Set(status);
}

std::string GSStringQueueIPC::GetString(std::size_t i)
{
	if (!CheckFileIndex(i))
		return "";

	char* s = strings + i * string_size;
	s[string_size - 1] = '\0';
	return std::string(s);
}

bool GSStringQueueIPC::PopulateStrings(const std::vector<std::string>& strings_in)
{
	if (strings_in.size() != num_strings)
	{
		Console.ErrorFmt("GSStringQueueIPC: PopulateStrings() incorrect number of strings ({} / {})", strings_in.size(), num_strings);
		return false;
	}
	for (std::size_t i = 0; i < num_strings; i++)
	{
		char* s = strings + i * string_size;
		CopyStringToBuffer(s, string_size, strings_in[i]);
	}
	return true;
}

void GSStringQueueIPC::SignalRunnerHeartbeat(std::size_t i)
{
	if (!CheckRunnerIndex(i))
		return;

	runner_heartbeats[i].Set(ALIVE);
}

bool GSStringQueueIPC::CheckRunnerHeartbeat(std::size_t i)
{
	if (!CheckRunnerIndex(i))
		return false;

	return runner_heartbeats[i].Get() == ALIVE;
}

void GSStringQueueIPC::ResetRunnerHeartbeat(std::size_t i)
{
	if (!CheckRunnerIndex(i))
		return;

	runner_heartbeats[i].Set(DEFAULT_RH);
}

void GSStringQueueIPC::SetStateChild(std::size_t i, ProcessState state)
{
	if (!CheckRunnerIndex(i))
		return;

	state_child[i].Set(static_cast<GSIntSharedMemory::ValType>(state));
}

void GSStringQueueIPC::SetStateParent(std::size_t i, ProcessState state)
{
	if (!CheckRunnerIndex(i))
		return;

	state_parent[i].Set(static_cast<GSIntSharedMemory::ValType>(state));
}

GSStringQueueIPC::ProcessState GSStringQueueIPC::GetStateChild(std::size_t i)
{
	if (!CheckRunnerIndex(i))
		return ERROR_PS;

	return static_cast<ProcessState>(state_child[i].Get());
}

GSStringQueueIPC::ProcessState GSStringQueueIPC::GetStateParent(std::size_t i)
{
	if (!CheckRunnerIndex(i))
		return ERROR_PS;

	return static_cast<ProcessState>(state_parent[i].Get());
}

std::size_t GSStringQueueIPC::GetTotalSize(std::size_t num_strings, std::size_t num_runners)
{
	return
		(string_size + sizeof(GSIntSharedMemory)) * num_strings + // File status and names.
		sizeof(GSIntSharedMemory) * (3 * num_runners + 1); // Runner parent/child/child-heartbeat status and head.
}
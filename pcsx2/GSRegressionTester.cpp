#include "GSRegressionTester.h"
#include "common/Assertions.h"
#include "common/Console.h"
#include "common/ScopedGuard.h"
#include "common/Timer.h"

#include <filesystem>
#include <thread>
#include <atomic>

#ifdef __WIN32__
#include "common/StringUtil.h"
#endif

static GSRegressionBuffer* regression_buffer; // Used by GS runner processes.

GSIntSharedMemory::ValType GSIntSharedMemory::CompareExchange(ValType expected, ValType desired)
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, desired, expected);
#else
	return std::atomic_compare_exchange_strong(&val, &expected, desired);
#endif
}

GSIntSharedMemory::ValType GSIntSharedMemory::Get()
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, 0, 0);
#else
	return 0; // not implemented
#endif
}

void GSIntSharedMemory::Set(ValType i)
{
#ifdef __WIN32__
	InterlockedExchange(&val, i);
#else
	// Not implemented
#endif
}

void GSIntSharedMemory::Init()
{
	Set(0);
}

std::size_t GSIntSharedMemory::GetTotalSize()
{
	return sizeof(GSIntSharedMemory);
}

bool GSSpinlockSharedMemory::LockWrite(bool block, GSIntSharedMemory* state)
{
	while (true)
	{
		if (state && state->Get() == GSRegressionBuffer::DONE)
		{
			return false; // Always fail when DONE.
		}

		if (lock.CompareExchange(WRITEABLE, WRITEABLE) == WRITEABLE)
		{
			return true;
		}

		if (!block)
		{
			return false; // Fail after 1 try when non-blocking.
		}

		std::this_thread::yield();
	}

	return false; // timeout
}

bool GSSpinlockSharedMemory::LockRead(bool block, GSIntSharedMemory* state)
{
	while (true)
	{
		if (state && state->Get() == GSRegressionBuffer::DONE)
		{
			return false; // Always fail when DONE.
		}

		if (lock.CompareExchange(READABLE, READABLE) == READABLE)
		{
			return true;
		}

		if (!block)
		{
			return false; // Fail after 1 try when non-blocking.
		}

		std::this_thread::yield();
	}
}

bool GSSpinlockSharedMemory::UnlockWrite()
{
	pxAssertRel(lock.Get() == WRITEABLE, "Trying to unlock write when not writeable.");
	return lock.CompareExchange(WRITEABLE, READABLE) == WRITEABLE;
}


bool GSSpinlockSharedMemory::UnlockRead()
{
	pxAssertRel(lock.Get() == READABLE, "Trying to unlock read when not readable.");
	return lock.CompareExchange(READABLE, WRITEABLE) == READABLE;
}

bool GSSpinlockSharedMemory::Writeable()
{
	return lock.Get() == WRITEABLE;
}

bool GSSpinlockSharedMemory::Readable()
{
	return lock.Get() == READABLE;
}

bool GSSpinlockSharedMemory::Lock(bool block, GSIntSharedMemory* state)
{
	while (true)
	{
		if (state && state->Get() == GSRegressionBuffer::DONE)
		{
			return false; // Always fail when DONE.
		}

		if (lock.CompareExchange(LOCKED, UNLOCKED) == UNLOCKED)
		{
			return true;
		}

		if (!block)
		{
			return false; // Fail after 1 try when non-blocking.
		}

		std::this_thread::yield();
	}
}

bool GSSpinlockSharedMemory::Unlock()
{
	return lock.CompareExchange(UNLOCKED, LOCKED) == LOCKED;
}

GSRegressionPacket* GSRegressionBuffer::GetPacketWrite(bool block)
{	
	if (!packets[packet_write % num_packets].lock.LockWrite(block, &state[TESTER]))
		return nullptr;

	return &packets[packet_write % num_packets];
}

GSRegressionPacket* GSRegressionBuffer::GetPacketRead(bool block)
{
	if (!packets[packet_read % num_packets].lock.LockRead(block, &state[TESTER]))
		return nullptr;

	return &packets[packet_read % num_packets];
}

void GSRegressionBuffer::DonePacketWrite()
{
	if (!packets[packet_write % num_packets].lock.UnlockWrite())
		pxFail("Unlock packet write is broken.");

	packet_write++;
}

void GSRegressionBuffer::DonePacketRead()
{
	if (!packets[packet_read % num_packets].lock.UnlockRead())
		pxFail("Unlock packet read is broken.");

	packet_read++;
}

GSDumpFileSharedMemory* GSRegressionBuffer::GetDumpWrite(bool block)
{
	if (!dumps[dump_write % num_dumps]->lock.LockWrite(block, &state[TESTER]))
		return nullptr;

	return dumps[dump_write % num_dumps];
}

GSDumpFileSharedMemory* GSRegressionBuffer::GetDumpRead(bool block)
{
	if (!dumps[dump_read % num_dumps]->lock.LockRead(block, &state[TESTER]))
		return nullptr;

	return dumps[dump_read % num_dumps];
}

std::size_t GSDumpFileSharedMemory::GetTotalSize(std::size_t dump_size)
{
	return sizeof(GSDumpFileSharedMemory) + dump_size;
}

void GSRegressionBuffer::DoneDumpWrite()
{
	if (!dumps[dump_write % num_dumps]->lock.UnlockWrite())
		pxFail("Unlock dump write is broken.");

	dump_write++;
}

void GSRegressionBuffer::DoneDumpRead()
{
	if (!dumps[dump_read % num_dumps]->lock.UnlockRead())
		pxFail("Unlock dump read is broken.");

	dump_read++;
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
	if (path.length() + 1 >= name_size)
	{
		Console.Warning("Name is too large for buffer.");
	}

	std::string pstr = std::filesystem::path(path).filename().string();
	strncpy_s(dst, name_size, pstr.c_str(), pstr.length() + 1);
	dst[name_size - 1] = '\0';
}

void GSRegressionPacket::SetImage(const void* src, int w, int h, int pitch, int bytes_per_pixel)
{
	if (src)
	{
		const std::size_t src_size = h * pitch;

		if (src_size > std::size(image.data))
		{
			Console.Warning("Image data is too large for regression packet.");
		}

		memcpy_s(image.data, std::size(image.data), src, src_size);
	}

	this->type = IMAGE;
	this->w = w;
	this->h = h;
	this->pitch = pitch;
	this->bytes_per_pixel = bytes_per_pixel;
}

void GSRegressionPacket::SetHWStat(const HWStat& hwstat)
{
	this->type = HWSTAT;
	this->hwstat = hwstat;
}

void GSRegressionPacket::Init()
{
	memset(this, 0, GetTotalSize());
};

std::size_t GSRegressionPacket::GetTotalSize()
{
	return sizeof(GSRegressionPacket);
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

// Only once before sharing. Not thread safe.
void GSDumpFileSharedMemory::Init(std::size_t dump_size)
{
	lock = {GSSpinlockSharedMemory::WRITEABLE};
	this->dump_size = dump_size;
	memset(GetPtrDump(), 0, dump_size);
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
	memcpy(name, str.c_str(), std::min(name_size - 1, str.length()));

	name[std::min(name_size - 1, str.length())] = '\0';
}

std::size_t GSRegressionBuffer::GetTotalSize(std::size_t num_packets, std::size_t dump_size)
{
	return num_packets * GSRegressionPacket::GetTotalSize() +
	       num_dumps * GSDumpFileSharedMemory::GetTotalSize(dump_size) +
	       num_states * GSIntSharedMemory::GetTotalSize();
}

bool GSRegressionBuffer::CreateFile_(const std::string& name, std::size_t num_packets, std::size_t dump_size)
{
	if (!shm.CreateFile_(name, GetTotalSize(num_packets, dump_size)))
		return false;

	// Non-shared memory initialization.
	Init(num_packets, dump_size);

	// All of these live in shared memory.
	for (int i = 0; i < num_packets; i++)
		packets[i].Init();
	for (int i = 0; i < num_dumps; i++)
		dumps[i]->Init(dump_size);
	for (int i = 0; i < 2; i++)
		state[i].Set(DEFAULT);

	return true;
}

void GSRegressionBuffer::Init(std::size_t num_packets, std::size_t dump_size)
{
	std::size_t packet_offset;
	std::size_t dump_file_offset[num_dumps];
	std::size_t state_offset;

	const std::size_t start_offset = reinterpret_cast<std::size_t>(shm.data);
	std::size_t curr_offset = start_offset;

	packet_offset = curr_offset;
	curr_offset += num_packets * GSRegressionPacket::GetTotalSize();

	for (int i = 0; i < num_dumps; i++)
	{
		dump_file_offset[i] = curr_offset;
		curr_offset += GSDumpFileSharedMemory::GetTotalSize(dump_size);
	}

	state_offset = curr_offset;
	curr_offset += num_states * GSIntSharedMemory::GetTotalSize();

	pxAssert(curr_offset - start_offset == GetTotalSize(num_packets, dump_size));

	packets = reinterpret_cast<GSRegressionPacket*>(packet_offset);
	for (int i = 0; i < num_dumps; i++)
		dumps[i] = reinterpret_cast<GSDumpFileSharedMemory*>(dump_file_offset[i]);
	state = reinterpret_cast<GSIntSharedMemory*>(state_offset);
	
	this->num_packets = num_packets;
	this->packet_write = 0;
	this->packet_read = 0;
	this->dump_size = dump_size;
	this->dump_write = 0;
	this->dump_read = 0;
	this->dump_name.clear();
}

void GSRegressionBuffer::Reset()
{
	for (int i = 0; i < num_packets; i++)
		packets[i].Init();
	for (int i = 0; i < num_dumps; i++)
		dumps[i]->Init(dump_size);
	for (int i = 0; i < 2; i++)
		state[i].Init();
}

bool GSRegressionBuffer::OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size)
{
	if (!shm.OpenFile(name, GetTotalSize(num_packets, dump_size)))
		return false;

	Init(num_packets, dump_size);

	return true;
}

bool GSRegressionBuffer::CloseFile()
{
	packets = nullptr;
	for (int i = 0; i < num_dumps; i++)
		dumps[i] = nullptr;
	state = nullptr;
	num_packets = 0;

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

bool GSIsRegressionTesting()
{
	return regression_buffer != nullptr;
}

/// Start regression testing within the producer/GS runner process.
void GSStartRegressionTest(GSRegressionBuffer* rpb, const std::string& fn, std::size_t num_packets, std::size_t dump_size)
{
	if (!rpb->OpenFile(fn, num_packets, dump_size))
	{
		pxFail("Unable to start regression test.");
		return;
	}

	Console.WriteLnFmt("Opened {} for regression testing.", fn);

	regression_buffer = rpb;
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
	return GSIsRegressionTesting() ? regression_buffer : nullptr;
}

template<int bytes_per_pixel>
static float RegressionCompareImagesImpl(const GSRegressionPacket* p1, const GSRegressionPacket* p2, int threshold)
{
	const u8* data1 = p1->image.data;
	const u8* data2 = p2->image.data;

	int num_diff_pixels = 0;

	for (int y = 0; y < p1->h; y++, data1 += p1->pitch, data2 += p2->pitch)
	{
		const u8* data1_row = data1;
		const u8* data2_row = data2;
		for (int x = 0; x < p1->w; x++, data1_row += bytes_per_pixel, data2_row += bytes_per_pixel)
		{
			if constexpr (bytes_per_pixel == 4)
			{				u32 d1 = *(u32*)data1;
				u32 d2 = *(u32*)data2;

				if (d1 != d2)
				{
					num_diff_pixels++;
				}
			}
			else if constexpr (bytes_per_pixel == 2)
			{
				u16 d1 = *(u16*)data1;
				u16 d2 = *(u16*)data2;

				if (d1 != d2)
				{
					num_diff_pixels++;
				}
			}
			else
			{
				pxFail("Failed");
			}
		}

		data1_row += p1->pitch;
		data2_row += p2->pitch;
	}

	int total_pixels = p1->w * p2->h;

	return static_cast<float>(num_diff_pixels) / total_pixels;
}

float RegressionCompareImages(const GSRegressionPacket* p1, const GSRegressionPacket* p2, int threshold)
{
	if (p1->w != p2->w || p1->h != p2->h || p1->bytes_per_pixel != p2->bytes_per_pixel)
		return 1.0f; // Formats are different.

	if (memcmp(p1->image.data, p2->image.data, p1->bytes_per_pixel * p1->h * p1->pitch) != 0)
		return 1.0f;
	else
		return 0.0f;

	if (p1->bytes_per_pixel == 4)
	{
		return RegressionCompareImagesImpl<4>(p1, p2, threshold);
	}
	else if (p1->bytes_per_pixel == 2)
	{
		return RegressionCompareImagesImpl<2>(p1, p2, threshold);
	}
	else
	{
		pxFail("");
		return NAN;
	}
}

bool GSProcess::Start(const std::string& command, bool detached)
{
#ifdef __WIN32__
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

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

	Console.WriteLnFmt("Created runner process (PID: {}) with command: '{}'", pi.dwProcessId, command);

	this->command = command;

	return true;
#else
	// Not implemented
#endif
}

bool GSProcess::IsRunning()
{
#ifdef __WIN32__
	DWORD status = WaitForSingleObject(pi.hProcess, 0);
	return status == WAIT_TIMEOUT;
#else
	// Not implemented
	return false;
#endif
}

int GSProcess::WaitForExit()
{
#ifdef __WIN32__
	return WaitForSingleObject(pi.hProcess, INFINITE);
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
	return false;
#endif
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

	Console.WriteLnFmt("Created regression packets file: {}", name);

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
	memset(data, 0, size);
}


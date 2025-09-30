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

static RegressionBuffer* regression_buffer; // Used by GS runner processes.

IntSharedMemory::ValType IntSharedMemory::CompareExchange(ValType expected, ValType desired)
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, desired, expected);
#else
	return std::atomic_compare_exchange_strong(&val, &expected, desired);
#endif
}

IntSharedMemory::ValType IntSharedMemory::Get()
{
#ifdef __WIN32__
	return InterlockedCompareExchange(&val, 0, 0);
#else
	return 0; // not implemented
#endif
}

void IntSharedMemory::Set(ValType i)
{
#ifdef __WIN32__
	InterlockedExchange(&val, i);
#else
	// Not implemented
#endif
}

void IntSharedMemory::Init()
{
	Set(0);
}

std::size_t IntSharedMemory::GetTotalSize()
{
	return sizeof(IntSharedMemory);
}

bool SpinlockSharedMemory::LockWrite(bool block, IntSharedMemory* done)
{
	while (1)
	{
		if (lock.CompareExchange(WRITEABLE, WRITEABLE) == WRITEABLE)
		{
			return true;
		}

		if (!block || (done && done->Get()))
		{
			return false;
		}

		std::this_thread::yield();
	}

	return false; // timeout
}

bool SpinlockSharedMemory::LockRead(bool block, IntSharedMemory* done)
{
	while (true)
	{
		if (lock.CompareExchange(READABLE, READABLE) == READABLE)
		{
			return true;
		}

		if (!block || (done && done->Get()))
		{
			return false;
		}

		std::this_thread::yield();
	}
}

bool SpinlockSharedMemory::UnlockWrite()
{
	pxAssertRel(lock.Get() == WRITEABLE, "Trying to unlock write when not writeable.");
	return lock.CompareExchange(WRITEABLE, READABLE) == WRITEABLE;
}


bool SpinlockSharedMemory::UnlockRead()
{
	pxAssertRel(lock.Get() == READABLE, "Trying to unlock read when not readable.");
	return lock.CompareExchange(READABLE, WRITEABLE) == READABLE;
}

bool SpinlockSharedMemory::Writeable()
{
	return lock.Get() == WRITEABLE;
}

bool SpinlockSharedMemory::Readable()
{
	return lock.Get() == READABLE;
}

bool SpinlockSharedMemory::Lock(bool block, IntSharedMemory* done)
{
	while (true)
	{
		if (lock.CompareExchange(LOCKED, UNLOCKED) == UNLOCKED)
		{
			return true;
		}

		if (!block || (done && done->Get()))
		{
			return false;
		}

		std::this_thread::yield();
	}
}

bool SpinlockSharedMemory::Unlock()
{
	return lock.CompareExchange(UNLOCKED, LOCKED) == LOCKED;
}

RegressionPacket* RegressionBuffer::GetPacketWrite(bool block)
{	
	if (!packets[packet_write % num_packets].lock.LockWrite(block, done[RUNNER]))
		return nullptr;

	return &packets[packet_write % num_packets];
}

RegressionPacket* RegressionBuffer::GetPacketRead(bool block)
{
	if (!packets[packet_read % num_packets].lock.LockRead(block, done[TESTER]))
		return nullptr;

	return &packets[packet_read % num_packets];
}

void RegressionBuffer::DonePacketWrite()
{
	if (!packets[packet_write % num_packets].lock.UnlockWrite())
		pxFail("Unlock packet write is broken.");

	packet_write++;
}

void RegressionBuffer::DonePacketRead()
{
	if (!packets[packet_read % num_packets].lock.UnlockRead())
		pxFail("Unlock packet read is broken.");

	packet_read++;
}

DumpFileSharedMemory* RegressionBuffer::GetDumpWrite(bool block)
{
	if (!dumps[dump_write % num_dumps]->lock.LockWrite(block, done[TESTER]))
		return nullptr;

	return dumps[dump_write % num_dumps];
}

DumpFileSharedMemory* RegressionBuffer::GetDumpRead(bool block)
{
	if (!dumps[dump_read % num_dumps]->lock.LockRead(block, done[RUNNER]))
		return nullptr;

	return dumps[dump_read % num_dumps];
}

std::size_t DumpFileSharedMemory::GetTotalSize(std::size_t dump_size)
{
	return sizeof(DumpFileSharedMemory) + dump_size;
}

void RegressionBuffer::DoneDumpWrite()
{
	if (!dumps[dump_write % num_dumps]->lock.UnlockWrite())
		pxFail("Unlock dump write is broken.");

	dump_write++;
}

void RegressionBuffer::DoneDumpRead()
{
	if (!dumps[dump_read % num_dumps]->lock.UnlockRead())
		pxFail("Unlock dump read is broken.");

	dump_read++;
}

void RegressionPacket::SetNamePacket(const std::string& path)
{
	SetName(name_packet, path);
}

void RegressionPacket::SetNameDump(const std::string& path)
{
	SetName(name_dump, path);
}

void RegressionPacket::SetName(char* dst, const std::string& path)
{
	if (path.length() + 1 >= name_size)
	{
		Console.Warning("Name is too large for buffer.");
	}

	std::string pstr = std::filesystem::path(path).filename().string();
	strncpy_s(dst, name_size, pstr.c_str(), pstr.length() + 1);
	dst[name_size - 1] = '\0';
}

void RegressionPacket::SetImageData(const void* src, int w, int h, int pitch, int bytes_per_pixel)
{
	if (src)
	{
		const std::size_t src_size = h * pitch;

		if (src_size > std::size(data))
		{
			Console.Warning("Image data is too large for regression packet.");
		}

		memcpy_s(data, std::size(data), src, src_size);
	}

	this->w = w;
	this->h = h;
	this->pitch = pitch;
	this->bytes_per_pixel = bytes_per_pixel;
}

void RegressionPacket::Init()
{
	memset(this, 0, GetTotalSize());
};

std::size_t RegressionPacket::GetTotalSize()
{
	return sizeof(RegressionPacket);
}

std::string RegressionPacket::GetNameDump()
{
	name_dump[std::size(name_dump) - 1] = '\0';

	return std::string(name_dump);
}

std::string RegressionPacket::GetNamePacket()
{
	name_packet[std::size(name_packet) - 1] = '\0';

	return std::string(name_packet);
}

u8* RegressionPacket::GetImageData()
{
	return data;
}

// Only once before sharing. Not thread safe.
void DumpFileSharedMemory::Init(std::size_t dump_size)
{
	lock = {SpinlockSharedMemory::WRITEABLE};
	this->dump_size = dump_size;
	memset(GetPtrDump(), 0, dump_size);
}

static std::size_t GetTotalSize(std::size_t dump_size)
{
	return sizeof(DumpFileSharedMemory) + dump_size;
}

void* DumpFileSharedMemory::GetPtrDump()
{
	return reinterpret_cast<u8*>(this) + sizeof(DumpFileSharedMemory);
}

std::size_t DumpFileSharedMemory::GetSizeDump()
{
	return dump_size;
}

void DumpFileSharedMemory::SetSizeDump(std::size_t size)
{
	dump_size = size;
}

std::string DumpFileSharedMemory::GetNameDump()
{
	name[std::size(name) - 1] = '\0';

	return std::string(name);
}

void DumpFileSharedMemory::SetNameDump(const std::string& str)
{
	memcpy(name, str.c_str(), std::min(name_size - 1, str.length()));

	name[std::min(name_size - 1, str.length())] = '\0';
}


void StatusSharedMemory::Init(std::size_t size)
{
	this->size = size;

	memset(GetStatusPtr(), 0, size);
}

std::string StatusSharedMemory::GetStatus()
{
	lock.Lock(true);

	ScopedGuard sg([&]() { lock.Unlock(); });

	GetStatusPtr()[size - 1] = '\0';

	std::string str(GetStatusPtr());

	return str;
}

void StatusSharedMemory::SetStatus(const std::string& status)
{
	lock.Lock();

	ScopedGuard sg([&]() { lock.Unlock(); });

	memcpy(GetStatusPtr(), status.c_str(), std::min(size - 1, status.length()));

	GetStatusPtr()[std::min(size - 1, status.length())] = '\0';
}

// Not thread safe.
char* StatusSharedMemory::GetStatusPtr()
{
	return reinterpret_cast<char*>(this) + sizeof(StatusSharedMemory);
}

std::size_t StatusSharedMemory::GetTotalSize(std::size_t size)
{
	return sizeof(StatusSharedMemory) + size;
}

std::size_t RegressionBuffer::GetTotalSize(std::size_t num_packets, std::size_t dump_size, std::size_t status_size)
{
	return num_packets * RegressionPacket::GetTotalSize() +
	       num_dumps * DumpFileSharedMemory::GetTotalSize(dump_size) +
	       2 * StatusSharedMemory::GetTotalSize(status_size) +
	       2 * IntSharedMemory::GetTotalSize();
}

bool RegressionBuffer::CreateFile_(const std::string& name, std::size_t num_packets,
	std::size_t dump_size, std::size_t status_size)
{
	if (!shm.CreateFile_(name, GetTotalSize(num_packets, dump_size, status_size)))
		return false;

	// Non-shared memory initialization.
	Init(num_packets, dump_size, status_size);

	// All of these live in shared memory.
	for (int i = 0; i < num_packets; i++)
		packets[i].Init();
	for (int i = 0; i < num_dumps; i++)
		dumps[i]->Init(dump_size);
	for (int i = 0; i < 2; i++)
		status[i]->Init(status_size);
	for (int i = 0; i < 2; i++)
		done[i]->Init();

	return true;
}

void RegressionBuffer::Init(std::size_t num_packets, std::size_t dump_size, std::size_t status_size)
{
	std::size_t packet_offset;
	std::size_t dump_file_offset[num_dumps];
	std::size_t status_offset[2];
	std::size_t done_offset[2];

	const std::size_t start_offset = reinterpret_cast<std::size_t>(shm.data);
	std::size_t curr_offset = start_offset;

	packet_offset = curr_offset;
	curr_offset += num_packets * RegressionPacket::GetTotalSize();

	for (int i = 0; i < num_dumps; i++)
	{
		dump_file_offset[i] = curr_offset;
		curr_offset += DumpFileSharedMemory::GetTotalSize(dump_size);
	}

	for (int i = 0; i < 2; i++)
	{
		status_offset[i] = curr_offset;
		curr_offset += StatusSharedMemory::GetTotalSize(status_size);
	}

	for (int i = 0; i < 2; i++)
	{
		done_offset[i] = curr_offset;
		curr_offset += IntSharedMemory::GetTotalSize();
	}

	pxAssert(curr_offset - start_offset == GetTotalSize(num_packets, dump_size, status_size));

	packets = reinterpret_cast<RegressionPacket*>(packet_offset);
	for (int i = 0; i < num_dumps; i++)
		dumps[i] = reinterpret_cast<DumpFileSharedMemory*>(dump_file_offset[i]);
	for (int i = 0; i < 2; i++)
		status[i] = reinterpret_cast<StatusSharedMemory*>(status_offset[i]);
	for (int i = 0; i < 2; i++)
		done[i] = reinterpret_cast<IntSharedMemory*>(done_offset[i]);
	
	this->num_packets = num_packets;
	this->packet_write = 0;
	this->packet_read = 0;
	this->dump_size = dump_size;
	this->dump_write = 0;
	this->dump_read = 0;
	this->dump_name.clear();
	this->status_size = status_size;
}

void RegressionBuffer::Reset()
{
	for (int i = 0; i < num_packets; i++)
		packets[i].Init();
	for (int i = 0; i < num_dumps; i++)
		dumps[i]->Init(dump_size);
	for (int i = 0; i < 2; i++)
		status[i]->Init(status_size);
	for (int i = 0; i < 2; i++)
		done[i]->Init();
}

bool RegressionBuffer::OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size,
	std::size_t status_size)
{
	if (!shm.OpenFile(name, GetTotalSize(num_packets, dump_size, status_size)))
		return false;

	Init(num_packets, dump_size, status_size);

	return true;
}

bool RegressionBuffer::CloseFile()
{
	packets = nullptr;
	for (int i = 0; i < num_dumps; i++)
		dumps[i] = nullptr;
	for (int i = 0; i < 2; i++)
		status[i] = nullptr;
	for (int i = 0; i < 2; i++)
		done[i] = nullptr;
	num_packets = 0;

	if (!shm.CloseFile())
		return false;

	return true;
}

bool RegressionBuffer::IsDoneRunner()
{
	return done[RUNNER]->Get();
}

bool RegressionBuffer::IsDoneTester()
{
	return done[TESTER]->Get();
}

void RegressionBuffer::SetDoneRunner(bool d)
{
	done[RUNNER]->Set(static_cast<IntSharedMemory::ValType>(d));
}

void RegressionBuffer::SetDoneTester(bool d)
{
	done[TESTER]->Set(static_cast<IntSharedMemory::ValType>(d));
}

std::string RegressionBuffer::GetStatus(u32 type)
{
	return status[type]->GetStatus();
};

std::string RegressionBuffer::GetStatusRunner()
{
	return GetStatus(RUNNER);
};

std::string RegressionBuffer::GetStatusTester()
{
	return GetStatus(TESTER);
};

void RegressionBuffer::SetStatus(const std::string& str, u32 type)
{
	status[type]->SetStatus(str);
};

void RegressionBuffer::SetStatusRunner(const std::string& str)
{
	SetStatus(str, RUNNER);
};

void RegressionBuffer::SetStatusTester(const std::string& str)
{
	SetStatus(str, TESTER);
};

void RegressionBuffer::SetNameDump(const std::string& name)
{
	dump_name = name;
}

std::string RegressionBuffer::GetNameDump()
{
	return dump_name;
}

bool IsRegressionTesting()
{
	return regression_buffer != nullptr;
}

/// Start regression testing within the producer/GS runner process.
void StartRegressionTest(RegressionBuffer* rpb, const std::string& fn, std::size_t num_packets,
	std::size_t dump_size, std::size_t status_size)
{
	if (!rpb->OpenFile(fn, num_packets, dump_size, status_size))
	{
		pxFail("Unable to start regression test.");
		return;
	}

	Console.WriteLnFmt("Successfully opened {} for regression testing.", fn);

	regression_buffer = rpb;
}

void EndRegressionTest()
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

RegressionBuffer* GetRegressionBuffer()
{
	return IsRegressionTesting() ? regression_buffer : nullptr;
}

template<int bytes_per_pixel>
static float RegressionCompareImagesImpl(const RegressionPacket* p1, const RegressionPacket* p2, int threshold)
{
	const u8* data1 = p1->data;
	const u8* data2 = p2->data;

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

float RegressionCompareImages(const RegressionPacket* p1, const RegressionPacket* p2, int threshold)
{
	if (p1->w != p2->w || p1->h != p2->h || p1->bytes_per_pixel != p2->bytes_per_pixel)
		return 1.0f; // Formats are different.

	if (memcmp(p1->data, p2->data, p1->bytes_per_pixel * p1->h * p1->pitch) != 0)
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

bool Process::Start(const std::string& command)
{
#ifdef __WIN32__
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

	std::wstring wcommand = StringUtil::UTF8StringToWideString(command);
	std::vector<wchar_t> wcommand_buf(wcommand.begin(), wcommand.end());
	wcommand_buf.push_back(L'\0');

	if (!CreateProcess(
			NULL,
			wcommand_buf.data(),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&si,
			&pi))
	{		Console.Error("Unable to create runner process with command: \"{}\"", command);
		return false;
	}

	Console.WriteLnFmt("Created runner process (PID: {}) with command: \"{}\"", pi.dwProcessId, command);

	this->command = command;

	return true;
#else
	// Not implemented
#endif
}

bool Process::IsRunning()
{
#ifdef __WIN32__
	DWORD status = WaitForSingleObject(pi.hProcess, 0);
	return status == WAIT_TIMEOUT;
#else
	// Not implemented
	return false;
#endif
}

int Process::WaitForExit()
{
#ifdef __WIN32__
	return WaitForSingleObject(pi.hProcess, INFINITE);
#else
	// Not implemented
	return false;
#endif
}

bool Process::Close()
{
#ifdef __WIN32__
	return CloseHandle(pi.hProcess) && CloseHandle(pi.hThread);
#else
	// Not implemented
	return false;
#endif
}

void Process::Terminate()
{
#ifdef __WIN32__
	TerminateProcess(pi.hProcess, EXIT_FAILURE);
#else
	// Not implemented
	return false;
#endif
}

// Windows defines CreateFile as a macro so use CreateFile_.
bool SharedMemoryFile::CreateFile_(const std::string& name, std::size_t size)
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

	data = static_cast<RegressionPacket*>(
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

	Console.WriteLnFmt("Successfully created regression packets file: {}", name);

	this->name = name;
	this->size = size;

	return true;
#else
	// Not implemented.
#endif
}

bool SharedMemoryFile::OpenFile(const std::string& name, std::size_t size)
{
	// Note: num_packets must match the value used in creation!
#ifdef __WIN32__
	handle = OpenFileMappingA(FILE_MAP_WRITE, FALSE, name.c_str());
	if (!handle)
	{
		Console.ErrorFmt("Not able to open file for regression packets: {}", name);
		return false;
	}

	data = static_cast<RegressionPacket*>(MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, size));
	if (!data)
	{
		Console.ErrorFmt("Unable to map regression packet file to memory: {}", name);
		CloseHandle(handle);
		return false;
	}

	Console.WriteLnFmt("Successfully opened/mapped regression packet file: {}", name);

	this->name = name;
	this->size = size;

	return true;
#else
	return false; // Not implemented.
#endif
}

bool SharedMemoryFile::CloseFile()
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

	Console.WriteLnFmt("Successfully closed/unmapped shared memory file: {}", name);

	name = "";
	handle = 0;
	data = nullptr;

	return true;
#else
	return false; // Not implemented.
#endif
}

void SharedMemoryFile::ResetFile()
{
	memset(data, 0, size);
}


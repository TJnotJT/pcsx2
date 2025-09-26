#include "GSRegressionTester.h"
#include "common/Assertions.h"
#include "common/Console.h"

#include <filesystem>
#include <thread>

#ifdef __WIN32__
#include "common/StringUtil.h"
#endif

static RegressionPacketBuffer* regression_buffer; // Used by GS runner processes.

RegressionPacket* RegressionPacketBuffer::GetPacketWrite(bool block)
{
	while (1)
	{
		RegressionPacket::State expected = RegressionPacket::Empty;
		if (packets[write % num_packets].state.compare_exchange_strong(expected, RegressionPacket::Writing,
				std::memory_order_acquire, std::memory_order_relaxed))
			break;

		if (!block)
			return nullptr;
		std::this_thread::yield();
	}

	return &packets[write % num_packets];
}

RegressionPacket* RegressionPacketBuffer::GetPacketRead(bool block)
{
	while (true)
	{
		if (packets[read % num_packets].state.load(std::memory_order_acquire) == RegressionPacket::Ready)
			break;

		if (!block)
			return nullptr;
		std::this_thread::yield();
	}

	return &packets[read % num_packets];
}

void RegressionPacketBuffer::DoneWrite()
{
	packets[write % num_packets].state.store(RegressionPacket::Ready, std::memory_order_release);
	write++;
}

void RegressionPacketBuffer::DoneRead()
{
	packets[read % num_packets].state.store(RegressionPacket::Empty, std::memory_order_release);
	read++;
}

void RegressionPacket::SetFilename(const char* fn)
{
	if (strnlen_s(fn, std::size(name)) >= std::size(name))
	{
		Console.Warning("File name is too large for regression packet.");
	}

	std::filesystem::path p = std::filesystem::path(fn).filename();
#ifdef __WIN32__
	std::u8string pstr = p.u8string();
	strncpy_s(name, reinterpret_cast<const char*>(pstr.c_str()), std::size(name));
#else
	strncpy_s(name, p.c_str(), std::size(name));
#endif
	Console.WriteLnFmt("New regression packet: {}", name);
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

int RegressionPacketBuffer::GetSize(int num_packets)
{
	return num_packets * (sizeof(RegressionPacket) + sizeof(int));
}

int RegressionPacketBuffer::GetReadyOffset(int num_packets)
{
	return num_packets * sizeof(RegressionPacket);
}

bool RegressionPacketBuffer::CreateFile_(const std::string& name, int num_packets)
{
#ifdef __WIN32__
	packets_h = CreateFileMappingA(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		num_packets * sizeof(RegressionPacket),
		name.c_str());

	if (!packets_h)
	{
		Console.ErrorFmt("Failed to create regression packets file: {}", name);
		return false;
	}

	packets = static_cast<RegressionPacket*>(
		MapViewOfFile(
			packets_h,
			FILE_MAP_WRITE,
			0,
			0,
			num_packets * sizeof(RegressionPacket)));

	if (!packets)
	{
		Console.ErrorFmt("Failed to map view of regressions packet file: {}", name);
		CloseHandle(packets_h);
		return false;
	}

	Console.WriteLnFmt("Successfully created regression packets file: {}", name);

	this->name = name;
	this->num_packets = num_packets;

	return true;
#else
	// Not implemented.
#endif
}

bool RegressionPacketBuffer::OpenFile(const std::string& name, int num_packets)
{
	// Note: num_packets must match the value used in creation!
#ifdef __WIN32__
	packets_h = OpenFileMappingA(FILE_MAP_WRITE, FALSE, name.c_str());
	if (!packets_h)
	{
		Console.ErrorFmt("Not able to open file for regression packets: {}", name);
		return false;
	}
	
	packets = static_cast<RegressionPacket*>(MapViewOfFile(packets_h, FILE_MAP_WRITE, 0, 0, num_packets * sizeof(RegressionPacket)));
	if (!packets)
	{
		Console.ErrorFmt("Unable to map regression packet file to memory: {}", name);
		CloseHandle(packets_h);
		return false;
	}

	Console.WriteLnFmt("Successfully opened/mapped regression packet file: {}", name);

	this->name = name;
	this->num_packets = num_packets;

	return true;
#else
	return false; // Not implemented.
#endif
}

bool RegressionPacketBuffer::CloseFile()
{
#ifdef __WIN32__
	if (!packets_h)
	{
		Console.Error("There is no regression packet handle to close.");
		return false;
	}

	if (!CloseHandle(packets_h))
	{
		Console.Error("Failed to close regression packet handle.");
		return false;
	}

	if (!packets)
	{
		Console.Error("There is no regression packets view to unmap.");
		return false;
	}

	if (!UnmapViewOfFile(packets))
	{
		Console.Error("Failed to unmap regression packets view.");
		return false;
	}

	Console.WriteLnFmt("Successfully closed/unmapped regression packets file: {}", name);

	name = "";
	packets_h = 0;
	packets = nullptr;

	return true;
#else
	return false; // Not implemented.
#endif
}

void RegressionPacketBuffer::ResetFile()
{
	memset(packets, 0, num_packets * sizeof(RegressionPacket));
}

bool IsRegressionTesting()
{
	return regression_buffer != nullptr;
}

/// Start regression testing within the producer/GS runner process.
void StartRegressionTest(RegressionPacketBuffer* rpb, const std::string& fn, int num_packets)
{
	if (!rpb->OpenFile(fn, num_packets))
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

RegressionPacketBuffer* GetRegressionPacketBuffer()
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
			{
				u32 d1 = *(u32*)data1;
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
	{
		Console.Error("Unable to create runner process with command: \"{}\"", command);
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
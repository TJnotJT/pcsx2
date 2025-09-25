#include "GSRegressionTester.h"
#include "common/Assertions.h"
#include "common/Console.h"

#include <filesystem>
#include <thread>

#ifdef __WIN32__
#include "common/StringUtil.h"
#endif

bool regression_testing = true;                 // Are we running a regression test.
RegressionPacketBuffer regression_buffer_write; // Used by GS runner producer processes.

#ifdef __WIN32__
HANDLE regression_packet_h;
#endif

RegressionPacket* RegressionPacketBuffer::GetPacketWrite(bool block)
{
	write %= num_packets;

	while (packets[write].ready.load(std::memory_order_acquire))
	{
		if (!block)
			return nullptr;
		std::this_thread::yield();
	}

	return &packets[write++];
}

RegressionPacket* RegressionPacketBuffer::GetPacketRead(bool block)
{
	read %= num_packets;

	while (!packets[read].ready.load(std::memory_order_acquire))
	{
		if (!block)
			return nullptr;
		std::this_thread::yield();
	}

	return &packets[read++];
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

	this->ready.store(true, std::memory_order_release);
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
		Console.Error("Failed to create regression packets file.");
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
		Console.Error("Failed to map view of regressions packet file.");
		CloseHandle(packets_h);
		return false;
	}

	Console.WriteLn("Successfully created regression packets file: {}", name.c_str());

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
		Console.ErrorFmt("Not able to open file for regression packets: {}", name.c_str());
		return false;
	}
	
	packets = static_cast<RegressionPacket*>(MapViewOfFile(packets_h, FILE_MAP_WRITE, 0, 0, num_packets * sizeof(RegressionPacket)));
	if (!packets)
	{
		Console.ErrorFmt("Unable to map regression packet file to memory: {}", name.c_str());
		CloseHandle(packets_h);
		return false;
	}

	Console.WriteLn("Successfully opened/mapped regression packet file: {}", name.c_str());

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

	Console.WriteLn("Successfully closed/unmapped regression packets file.");
	return true;
#else
	return false; // Not implemented.
#endif
}


/// Start regression testing within the producer/GS runner process.
void StartRegressionTest(const std::string& fn, int num_packets)
{
	if (!regression_buffer_write.OpenFile(fn, num_packets))
	{
		pxFail("Unable to start regression test.");
		return;
	}

	regression_testing = true;
}

void EndRegressionTest()
{
	if (!regression_buffer_write.CloseFile())
	{
		pxFail("Unable to end regression test.");
		return;
	}
}

RegressionPacket* GetRegressionPacketWrite()
{
	return regression_testing ? regression_buffer_write.GetPacketWrite() : nullptr;
}
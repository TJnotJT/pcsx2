#pragma once

#include <mutex>

#ifdef __WIN32__
#include <windows.h>
#endif

struct RegressionPacket
{
	enum State
	{
		Empty,
		Writing,
		Ready
	};
	std::atomic<State> state; // Contains data to be consumed.
	char name[4096];
	int size, w, h, pitch, bytes_per_pixel;
	u8 data[1024 * 1024 * 4];

	void SetFilename(const char* fn);
	void SetImageData(const void* src, int w, int h, int pitch, int bytes_per_pixel);
};

struct SharedMemoryFile
{
	std::string name = "";
	void* data = nullptr;
	std::size_t size;
#ifdef __WIN32__
	HANDLE handle; // Handle to shared memory.
#else
	// Not implemented.
#endif

	// Windows defines CreateFile as a macro so use CreateFile_.
	bool CreateFile_(const std::string& name, std::size_t size);
	bool OpenFile(const std::string& name, std::size_t size);
	bool CloseFile();
	void ResetFile();
};

struct RegressionSharedMemoryFile
{
	SharedMemoryFile shm;
	RegressionPacket* packets;
	void* dump_file[2];
	char* status; // 4096 bytes

	static std::size_t GetSize(std::size_t num_packets, std::size_t dump_size);
};

/// Ring buffer of regression packets.
struct RegressionBuffer
{
	SharedMemoryFile shm;

	RegressionPacket* packets = nullptr;
	std::size_t num_packets = 0;
	std::atomic<std::size_t> read = 0;  // read index.
	std::atomic<std::size_t> write = 0; // write index;

	void* dump_file[2];
	std::size_t dump_size;

	char* status;
	std::size_t status_size;

	int frames = 0;
	int draws = 0;
	int render_passes = 0;
	int barriers = 0;
	int copies = 0;
	int uploads = 0;
	int readbacks = 0;

	// Windows defines CreateFile as a macro so use CreateFile_.
	bool CreateFile_(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);
	bool OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);
	bool CloseFile();
	void ResetFile();
	void SetSizesPointers(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);

	RegressionPacket* GetPacketWrite(bool block = true);
	RegressionPacket* GetPacketRead(bool block = false);
	void DoneWrite();
	void DoneRead();

	static std::size_t GetSize(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);
};



bool IsRegressionTesting();
void StartRegressionTest(RegressionPacketBuffer* rpb, const std::string& fn, int num_packets);
void EndRegressionTest();
RegressionPacketBuffer* GetRegressionPacketBuffer();

enum ImageCompare
{
	BINARY,
	BINARY_THRESHOLD,
	FRACTIONAL,
	FRACTIONAL_THRESHOLD
};

float RegressionCompareImages(const RegressionPacket* p1, const RegressionPacket* p2, int threshold);

struct Process
{
	std::string command;
#ifdef __WIN32__
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
#else
	// Not implemented.
#endif
	bool Start(const std::string& command);
	bool IsRunning();
	int WaitForExit();
	bool Close();
};
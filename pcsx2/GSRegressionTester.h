#pragma once

#include <mutex>

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef __WIN32__
struct SpinlockSharedMemory
{
	volatile LONG lock = WRITEABLE;

	// For producer/consumer.
	enum : LONG
	{
		WRITEABLE = 0,
		READABLE = 1
	};

	bool LockWrite(bool block = true);
	bool LockRead(bool block = false);
	bool UnlockWrite();
	bool UnlockRead();
	bool Writeable();
	bool Readable();

	// For lock/unlock.
	enum : LONG
	{
		UNLOCKED = 0,
		LOCKED = 1
	};

	bool Lock(bool block = false);
	bool Unlock();
};
#else
// Not implemented
#endif

struct RegressionPacket
{
	static constexpr std::size_t name_size = 4096;
	static constexpr std::size_t data_size = 4 * 1024 * 1024;

	SpinlockSharedMemory lock;
	char name_dump[name_size];
	char name_packet[name_size];
	int size, w, h, pitch, bytes_per_pixel;
	u8 data[data_size];

	// Must only be called when owned by consumer.
	void SetNameDump(const char* name);
	void SetNamePacket(const char* name);
	void SetImageData(const void* src, int w, int h, int pitch, int bytes_per_pixel);

	// Must only be called when owned by producer.
	std::string GetNameDump();
	std::string GetNamePacket();
	u8* GetImageData();

	// Call only once before sharing. Not thread safe.
	void Init();

	// Static
	static std::size_t GetSize();
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

struct DumpFileSharedMemory
{
	static constexpr std::size_t name_size = 4096;

	SpinlockSharedMemory lock;
	char name[name_size];

	// Note: not the true dump size; just size of buffer.
	// The actual dump size is serialized in the buffer.
	std::size_t dump_size;

	// Call only once before sharing. Not thread safe.
	void Init(std::size_t dump_size);

	// Call by owner.
	void* GetDump();

	// Call by reader when owned.
	std::size_t GetDumpSize();
	std::string GetName();

	// Call by writer when owned.
	void SetDumpSize(std::size_t size);
	void SetName(const std::string& str);

	// Static.
	static std::size_t GetSize(std::size_t dump_size);
};

struct StatusSharedMemory
{
	SpinlockSharedMemory lock;
	std::size_t size;

	// Call only once before sharing. Not thread safe.
	void Init(std::size_t size);

	// Thread safe.
	std::string GetStatus();
	void SetStatus(const std::string& status);

	// Not thread safe.
	char* GetStatusRaw();

	// Static.
	static std::size_t GetSize(std::size_t size);
};

/// Ring buffer of regression packets, dump files, and status.
struct RegressionBuffer
{
	SharedMemoryFile shm;

	RegressionPacket* packets = nullptr;
	std::size_t num_packets = 0;
	std::size_t packet_write = 0;
	std::size_t packet_read = 0;

	static constexpr std::size_t num_dumps = 2;
	DumpFileSharedMemory* dumps[num_dumps];
	std::size_t dump_write = 0;
	std::size_t dump_read = 0;

	StatusSharedMemory* status;

	// Call only once before sharing.
	bool CreateFile_(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);

	// Call only once by child.
	bool OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);

	// Call only once by parent.
	bool CloseFile();

	// Call only once to initialize.
	void SetSizesPointers(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);

	// Thread safe; acquire ownership.
	RegressionPacket* GetPacketWrite(bool block = true);
	RegressionPacket* GetPacketRead(bool block = false);

	// Call only by owner to release ownership.
	bool DoneWritePacket();
	bool DoneReadPacket();

	// Thread safe; acquire ownership.
	DumpFileSharedMemory* GetDumpWrite(bool block = true);
	DumpFileSharedMemory* GetDumpRead(bool block = false);

	// Call only by owner to release ownership.
	bool DoneDumpWrite();
	bool DoneDumpRead();

	// Thread safe.
	std::string GetStatus();
	void SetStatus(const std::string& str);

	// Static.
	static std::size_t GetSize(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);
};

bool IsRegressionTesting();
void StartRegressionTest(RegressionBuffer* rpb, const std::string& fn, std::size_t num_packets,
	std::size_t dump_size, std::size_t status_size);
void EndRegressionTest();
RegressionBuffer* GetRegressionBuffer();

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

#ifdef __WIN32__
struct Mutex
{
	HANDLE handle;
	// Not implemented
	Mutex();
	~Mutex();

	void Lock();
	void Unlock();
};
#else
// Not implemented
#endif
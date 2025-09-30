#pragma once

#include <mutex>

#ifdef __WIN32__
#include <windows.h>
#endif

struct IntSharedMemory
{

#ifdef __WIN32__
	using ValType = LONG;
#else
	using ValType = long;
#endif
	ValType val;
	ValType CompareExchange(ValType expected, ValType desired);
	ValType Get();
	void Set(ValType i);
	void Init();
	static std::size_t GetTotalSize();
};

#ifdef __WIN32__
struct SpinlockSharedMemory
{
	// For producer/consumer semantics.
	enum : LONG
	{
		WRITEABLE = 0,
		READABLE = 1
	};

	IntSharedMemory lock;

	bool LockWrite(bool block = true, IntSharedMemory* done = nullptr);
	bool LockRead(bool block = false, IntSharedMemory* done = nullptr);
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

	bool Lock(bool block = false, IntSharedMemory* done = nullptr);
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

	// Call by owner.
	void SetNameDump(const std::string& name);
	void SetNamePacket(const std::string& name);
	void SetName(char* dst, const std::string& name); // Helper (private)
	void SetImageData(const void* src, int w, int h, int pitch, int bytes_per_pixel);
	std::string GetNameDump();
	std::string GetNamePacket();
	u8* GetImageData();

	// Call only once before sharing. Not thread safe.
	void Init();

	// Static
	static std::size_t GetTotalSize();
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
	void* GetPtrDump();
	std::size_t GetSizeDump();
	std::string GetNameDump();
	void SetSizeDump(std::size_t size);
	void SetNameDump(const std::string& str);

	// Static.
	static std::size_t GetTotalSize(std::size_t dump_size);
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
	char* GetStatusPtr();

	// Static.
	static std::size_t GetTotalSize(std::size_t size);
};

/// Ring buffer of regression packets, dump files, and status.
struct RegressionBuffer
{
	static constexpr const char* RUNNING = "RUNNING";
	static constexpr const char* WAIT_DUMP = "WAIT_DUMP";
	static constexpr const char* WRITING_DATA = "WRITING_DATA";
	static constexpr const char* DONE = "DONE";

	enum
	{
		RUNNER = 0,
		TESTER
	};

	SharedMemoryFile shm;

	RegressionPacket* packets = nullptr;
	std::size_t num_packets = 0;
	std::size_t packet_write = 0;
	std::size_t packet_read = 0;

	static constexpr std::size_t num_dumps = 2;
	DumpFileSharedMemory* dumps[num_dumps];
	std::size_t dump_write = 0;
	std::size_t dump_read = 0;
	std::size_t dump_size = 0;
	std::string dump_name;

	StatusSharedMemory* status[2];
	std::size_t status_size;

	IntSharedMemory* done[2];

	// Call only once before sharing.
	bool CreateFile_(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);

	// Call only once by child.
	bool OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size,
		std::size_t status_size);

	// Call only once by parent.
	bool CloseFile();

	// Call only once to initialize.
	void Init(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);
	void Reset();

	// Thread safe; acquire ownership.
	RegressionPacket* GetPacketWrite(bool block = true);
	RegressionPacket* GetPacketRead(bool block = false);

	// Call only by owner to release ownership.
	void DonePacketWrite();
	void DonePacketRead();

	// Thread safe; acquire ownership.
	DumpFileSharedMemory* GetDumpWrite(bool block = true);
	DumpFileSharedMemory* GetDumpRead(bool block = false);

	// Call only by owner to release ownership.
	void DoneDumpWrite();
	void DoneDumpRead();

	// Thread safe.
	bool IsDoneRunner();
	bool IsDoneTester();
	void SetDoneRunner(bool done);
	void SetDoneTester(bool done);
	std::string GetStatus(u32 type);
	void SetStatus(const std::string& str, u32 type);
	std::string GetStatusRunner();
	std::string GetStatusTester();
	void SetStatusRunner(const std::string& str);
	void SetStatusTester(const std::string& str);

	// Access only local data.
	void SetNameDump(const std::string& name);
	std::string GetNameDump();

	// Static.
	static std::size_t GetTotalSize(std::size_t num_packets, std::size_t dump_size, std::size_t status_size);
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
	void Terminate();
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
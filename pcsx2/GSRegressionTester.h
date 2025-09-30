#pragma once

#include <mutex>

#ifdef __WIN32__
#include <windows.h>
#endif

// Atomic integer for inter-process shared memory since std::atomic is not guaranteed across processes.
struct GSIntSharedMemory
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

// Spinlock using the inter-process atomic.
#ifdef __WIN32__
struct GSSpinlockSharedMemory
{
	// For producer/consumer semantics.
	enum : LONG
	{
		WRITEABLE = 0,
		READABLE = 1
	};

	GSIntSharedMemory lock;

	bool LockWrite(bool block = true, GSIntSharedMemory* done = nullptr);
	bool LockRead(bool block = false, GSIntSharedMemory* done = nullptr);
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

	bool Lock(bool block = false, GSIntSharedMemory* done = nullptr);
	bool Unlock();
};
#else
// Not implemented
#endif

// Packet holding data uploaded by runners for tester to consume and diff.
// Lives in shared memory.
struct GSRegressionPacket
{
	static constexpr std::size_t name_size = 4096;
	static constexpr std::size_t image_size = 4 * 1024 * 1024;

	enum : u32
	{
		IMAGE,
		HWSTAT
	};

	struct alignas(32) HWStat
	{
		std::size_t frames;
		std::size_t draws;
		std::size_t render_passes;
		std::size_t barriers;
		std::size_t copies;
		std::size_t uploads;
		std::size_t readbacks;

		bool operator==(const HWStat& other)
		{
			return memcmp(this, &other, sizeof(HWStat)) == 0;
		};

		bool operator!=(const HWStat& other)
		{
			return !operator==(other);
		};
	};

	struct alignas(32) Image
	{
		std::size_t size;
		std::size_t w;
		std::size_t h;
		std::size_t pitch;
		std::size_t bytes_per_pixel;
		u8 data[image_size];
	};

	GSSpinlockSharedMemory lock;
	u32 type;
	char name_dump[name_size];
	char name_packet[name_size];
	int size, w, h, pitch, bytes_per_pixel;
	union
	{
		Image image;
		HWStat hwstat;
	};

	// Call by owner.
	void SetNameDump(const std::string& name);
	void SetNamePacket(const std::string& name);
	void SetName(char* dst, const std::string& name); // Helper (private)
	void SetImage(const void* src, int w, int h, int pitch, int bytes_per_pixel);
	void SetHWStat(const HWStat& hwstat);
	std::string GetNameDump();
	std::string GetNamePacket();

	// Call only once before sharing. Not thread safe.
	void Init();

	// Static
	static std::size_t GetTotalSize();
};

// Cross-platform shared memory file for regression testing.
struct GSSharedMemoryFile
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

// GSDumpFile that lives in shared memory. Allows the tester to read/decode dump
// files from disk once and upload for runners.
struct GSDumpFileSharedMemory
{
	static constexpr std::size_t name_size = 4096;

	GSSpinlockSharedMemory lock;
	char name[name_size];

	// Note: not the true dump size; just size of buffer.
	// The actual dump size is obtained by parsing the buffer.
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

// Ring buffers of regression packets and dump files.
struct GSRegressionBuffer
{
	enum : u32
	{
		RUNNER = 0,
		TESTER = 1
	};

	enum : u32
	{
		DEFAULT = 0,
		WAIT_DUMP,
		WRITE_DATA,
		DONE
	};

	GSSharedMemoryFile shm;

	GSRegressionPacket* packets = nullptr;
	std::size_t num_packets = 0;
	std::size_t packet_write = 0;
	std::size_t packet_read = 0;

	static constexpr std::size_t num_dumps = 2;
	GSDumpFileSharedMemory* dumps[num_dumps]; // Must use array of pointer because object size is unknown at compile time.
	std::size_t dump_write = 0;
	std::size_t dump_read = 0;
	std::size_t dump_size = 0;
	std::string dump_name;

	static constexpr std::size_t num_states = 2;
	GSIntSharedMemory* state; // Two states owned by runner and tester.

	// Call only once before sharing.
	bool CreateFile_(const std::string& name, std::size_t num_packets, std::size_t dump_size);

	// Call only once by child.
	bool OpenFile(const std::string& name, std::size_t num_packets, std::size_t dump_size);

	// Call only once by parent.
	bool CloseFile();

	// Call only once to initialize.
	void Init(std::size_t num_packets, std::size_t dump_size);
	void Reset();

	// Thread safe; acquire ownership.
	GSRegressionPacket* GetPacketWrite(bool block = true);
	GSRegressionPacket* GetPacketRead(bool block = false);

	// Call only by owner to release ownership.
	void DonePacketWrite();
	void DonePacketRead();

	// Thread safe; acquire ownership.
	GSDumpFileSharedMemory* GetDumpWrite(bool block = true);
	GSDumpFileSharedMemory* GetDumpRead(bool block = false);

	// Call only by owner to release ownership.
	void DoneDumpWrite();
	void DoneDumpRead();

	// Thread safe.
	u32 GetState(u32 which);
	void SetState(u32 which, u32 state);
	u32 GetStateRunner();
	u32 GetStateTester();
	void SetStateRunner(u32 state);
	void SetStateTester(u32 state);

	// Access only local data.
	void SetNameDump(const std::string& name);
	std::string GetNameDump();

	// Static.
	static std::size_t GetTotalSize(std::size_t num_packets, std::size_t dump_size);
};

// To be call by the runner process when in regression test mode.
bool GSIsRegressionTesting();
void GSStartRegressionTest(GSRegressionBuffer* rpb, const std::string& fn, std::size_t num_packets, std::size_t dump_size);
void GSEndRegressionTest();
GSRegressionBuffer* GSGetRegressionBuffer();

enum ImageCompare
{
	BINARY,
	BINARY_THRESHOLD,
	FRACTIONAL,
	FRACTIONAL_THRESHOLD
};

float RegressionCompareImages(const GSRegressionPacket* p1, const GSRegressionPacket* p2, int threshold);

// Cross-platform process.
struct GSProcess
{
	std::string command;
#ifdef __WIN32__
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
#else
	// Not implemented.
#endif
	bool Start(const std::string& command, bool detached);
	bool IsRunning();
	int WaitForExit();
	bool Close();
	void Terminate();
};

#pragma once

#ifdef __WIN32__
#include <windows.h>
#endif

struct alignas(32) RegressionPacket
{
	std::atomic<bool> ready; // Contains data to be consumed.
	char name[4096];
	int size, w, h, pitch, bytes_per_pixel;
	u8 data[1024 * 1024 * 4];

	void SetFilename(const char* fn);
	void SetImageData(const void* src, int w, int h, int pitch, int bytes_per_pixel);
};

/// Ring buffer of regression packets.
struct alignas(32) RegressionPacketBuffer
{
	std::string name;
#ifdef __WIN32__
	HANDLE packets_h; // Handle to shared memory.
#else
	// Not implemented.
#endif
	RegressionPacket* packets = nullptr;
	int num_packets = 0;
	int read = 0;  // read index.
	int write = 0; // write index;

	int frames = 0;
	int draws = 0;
	int render_passes = 0;
	int barriers = 0;
	int copies = 0;
	int uploads = 0;
	int readbacks = 0;

	// Windows defines CreateFile as a macro so use CreateFile_.
	bool CreateFile_(const std::string& name, int num_packets);
	bool OpenFile(const std::string& name, int num_packets);
	bool CloseFile();
	void ResetFile();

	RegressionPacket* GetPacketWrite(bool block = true);
	RegressionPacket* GetPacketRead(bool block = false);
};

bool IsRegressionTesting();
void StartRegressionTest(RegressionPacketBuffer* rpb, const std::string& fn, int num_packets);
void EndRegressionTest();
RegressionPacket* GetRegressionPacketWrite();

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
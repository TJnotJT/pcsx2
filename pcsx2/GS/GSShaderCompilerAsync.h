#pragma once

#include "common/Pcsx2Defs.h"
#include "common/Timer.h"

#include <thread>
#include <mutex>

class GSShaderCompilerAsync
{
public:
	struct GSCompileJob
	{
		bool done = false;
		float compile_time_ms = 0.0f; // Compile time in ms for debugging.
		u32 thread_id = UINT_MAX; // Thread ID for debugging.
	};

	GSShaderCompilerAsync(u32 num_threads = 1, u32 check_latency_ms = 20);
	virtual ~GSShaderCompilerAsync();

	bool IsJobQueueFull();
	u32 GetCompletedJobs();
	void StartCompileJobAsync(GSCompileJob* job);

protected:
	virtual void DoCompileJobSync(GSCompileJob* job) = 0;

private:
	void StopWorkerThreads();

	static constexpr size_t MAX_JOBS = 1024;

	std::vector<GSCompileJob*> m_job_queue;
	std::vector<Common::Timer> m_job_timer_queue;

	u32 m_job_head = 0; // head <= acquire <= tail
	u32 m_job_acquire = 0;
	u32 m_job_tail = 0;
	u32 m_queued_jobs = 0; // Count from head to tail
	u32 m_acquired_jobs = 0; // Count from acquire to tail

	Common::Timer m_check_timer;
	bool m_check_timer_init = false;
	u32 m_check_latency_ms = 20;

	std::vector<std::thread> m_worker_threads;
	std::mutex m_mutex;
	std::condition_variable m_worker_cv;
	bool m_workers_stop = false;
	bool m_workers_started = false;

	void WorkerThreadFunc(u32 thread_id);
};

struct GSAsyncReturn
{
private:
	const bool enabled; // Enable async return.
	bool async_return; // Call returned after dispatching async processing.
public:
	GSAsyncReturn(bool enabled) : enabled(enabled), async_return(false) {}

	static bool Enabled(GSAsyncReturn* async)
	{
		return async && async->enabled;
	}

	static void ClearAsync(GSAsyncReturn* async)
	{
		if (async)
			async->async_return = false;
	}

	static void SetAsync(GSAsyncReturn* async)
	{
		if (async)
			async->async_return = true;
	}

	static bool IsAsync(GSAsyncReturn* async)
	{
		return async && async->enabled && async->async_return;
	}
}

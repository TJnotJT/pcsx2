#pragma once

#include "common/Pcsx2Defs.h"
#include "common/Timer.h"

#include <memory>
#include <thread>
#include <mutex>

class GSCompileJob
{
public:
	enum JobStatus
	{
		NONE,
		DONE_COMPILING,
		DONE_CACHING,
	};

	enum JobType
	{
		SHADER,
		PIPELINE,
	};

	GSCompileJob(JobType type) : m_job_type(type) {}

	bool IsShaderJob() { return m_job_type == SHADER; }
	bool IsPipelineJob() { return m_job_type == PIPELINE; }

	virtual ~GSCompileJob() {}

	bool IsDoneCompiling() const { return m_status.load(std::memory_order_acquire) == DONE_COMPILING; }
	void SetDoneCompiling() { m_status.store(DONE_COMPILING, std::memory_order_release); }
	bool IsDoneCaching() const { return m_status.load(std::memory_order_acquire) == DONE_CACHING; }
	void SetDoneCaching() { m_status.store(DONE_CACHING, std::memory_order_release); }
	float GetCompileTime() const { return m_compile_time_ms; }
	void SetCompileTime(float ms) { m_compile_time_ms = ms; }
	u32 GetThreadID() const { return m_thread_id; }
	void SetThreadID(u32 id) { m_thread_id = id; }
private:
	JobType m_job_type;
	std::atomic<JobStatus> m_status = NONE;
	float m_compile_time_ms = 0.0f; // Compile time in ms for debugging.
	u32 m_thread_id = UINT_MAX; // Thread ID for debugging.
};

class GSShaderCompilerAsync
{
public:
	GSShaderCompilerAsync(u32 num_threads, u32 check_latency_ms);
	virtual ~GSShaderCompilerAsync();

	bool IsJobQueueFull();
	void GetCompletedJobs(std::vector<GSCompileJob*>& jobs);
	void StartCompileJobAsync(GSCompileJob* job);

protected:
	virtual void DoCompileJobSync(GSCompileJob* job, u32 thread_id) = 0;
	virtual void OnWorkersStarted() {}

	size_t GetNumThreads() { return m_worker_threads.size(); }

private:
	void StopWorkerThreads();

	static constexpr size_t MAX_JOBS = 1024;

	// GSDevice and the shader cache own the jobs so use raw pointers.
	std::vector<GSCompileJob*> m_job_queue;
	std::vector<Common::Timer> m_job_timer_queue;
	std::deque<GSCompileJob*> m_overflow_job_queue; // Should hardly be used.

	u32 m_job_head = 0; // head <= acquire <= tail
	u32 m_job_acquire = 0;
	u32 m_job_tail = 0;
	u32 m_queued_jobs = 0; // Count from head to tail
	u32 m_acquired_jobs = 0; // Count from acquire to tail

	Common::Timer m_check_timer;
	u32 m_check_latency_ms = 20;

	std::vector<std::thread> m_worker_threads;
	std::mutex m_mutex;
	std::condition_variable m_worker_cv;
	bool m_workers_stop = false;
	bool m_workers_started = false;

	void WorkerThreadFunc(u32 thread_id);
};

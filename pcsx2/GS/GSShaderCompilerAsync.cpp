#include "common/Assertions.h"

#include "GS/GSShaderCompilerAsync.h"

GSShaderCompilerAsync::GSShaderCompilerAsync(u32 num_threads, u32 check_latency_ms)
	: m_check_latency_ms(check_latency_ms)
{
	m_job_queue.resize(MAX_JOBS);
	m_job_timer_queue.resize(MAX_JOBS);
	m_worker_threads.resize(num_threads);
}

bool GSShaderCompilerAsync::IsJobQueueFull()
{
	std::unique_lock lock(m_mutex);

	return m_queued_jobs == MAX_JOBS;
}

u32 GSShaderCompilerAsync::GetCompletedJobs()
{
	if (static_cast<u32>(m_check_timer.GetTimeMilliseconds()) < m_check_latency_ms)
		return 0;

	m_check_timer.Reset();

	std::lock_guard lock(m_mutex);

	u32 completed_jobs = 0;
	while (m_acquired_jobs > 0 && m_job_queue[m_job_head]->done)
	{
		m_job_queue[m_job_head]->compile_time_ms =
			static_cast<float>(m_job_timer_queue[m_job_head].GetTimeMilliseconds());
		completed_jobs++;

		m_job_queue[m_job_head] = nullptr;

		m_job_head = (m_job_head + 1) % MAX_JOBS;

		pxAssert(m_acquired_jobs <= m_queued_jobs);
		m_queued_jobs--;
		m_acquired_jobs--;
	}

	return completed_jobs;
}

void GSShaderCompilerAsync::StartCompileJobAsync(GSCompileJob* job)
{
	if (!m_workers_started)
	{
		m_workers_started = true;
		u32 thread_id = 0;
		for (std::thread& t : m_worker_threads)
			t = std::thread([this, id = thread_id++]() { WorkerThreadFunc(id); });

		OnWorkersStarted();

		m_check_timer.Reset();
	}

	std::lock_guard lock(m_mutex);

	if (m_queued_jobs < MAX_JOBS)
	{
		m_job_queue[m_job_tail] = job;
		m_job_queue[m_job_tail]->done = false;
		m_job_timer_queue[m_job_tail].Reset();

		m_job_tail = (m_job_tail + 1) % MAX_JOBS;
		m_queued_jobs++;

		m_worker_cv.notify_one();
	}
}

void GSShaderCompilerAsync::WorkerThreadFunc(u32 thread_id)
{
	while (true)
	{
		// Acquire a queued job.
		u32 curr = 0;
		{
			std::unique_lock lock(m_mutex);

			m_worker_cv.wait(lock, [&]() {
				return m_workers_stop || m_acquired_jobs < m_queued_jobs;
			});

			if (m_workers_stop)
				return;

			curr = m_job_acquire;

			m_job_acquire = (m_job_acquire + 1) % MAX_JOBS;
			m_acquired_jobs++;
		}

		// Compile the job.
		DoCompileJobSync(m_job_queue[curr], thread_id);
		m_job_queue[curr]->thread_id = thread_id; // For debugging

		// Release the completed job.
		{
			std::lock_guard lock(m_mutex);

			m_job_queue[curr]->done = true;
		}
	}
}

void GSShaderCompilerAsync::StopWorkerThreads()
{
	// Set stop flag.
	{
		std::lock_guard lock(m_mutex);

		m_workers_stop = true;

		m_worker_cv.notify_all();
	}

	// Join threads.
	for (std::thread& t : m_worker_threads)
	{
		if (t.joinable())
			t.join();
	}
}

GSShaderCompilerAsync::~GSShaderCompilerAsync()
{
	StopWorkerThreads();
}

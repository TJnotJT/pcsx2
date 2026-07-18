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

void GSShaderCompilerAsync::GetCompletedJobs(std::vector<GSCompileJob*>& jobs)
{
	if (static_cast<u32>(m_check_timer.GetTimeMilliseconds()) < m_check_latency_ms)
		return;

	m_check_timer.Reset();

	std::lock_guard lock(m_mutex);

	while (m_acquired_jobs > 0 && m_job_queue[m_job_head]->IsDoneCompiling())
	{
		GSCompileJob* job = m_job_queue[m_job_head];

		job->SetCompileTime(static_cast<float>(m_job_timer_queue[m_job_head].GetTimeMilliseconds()));

		jobs.push_back(job);

		m_job_queue[m_job_head] = nullptr;

		m_job_head = (m_job_head + 1) % MAX_JOBS;

		pxAssert(m_acquired_jobs <= m_queued_jobs);
		m_queued_jobs--;
		m_acquired_jobs--;
	}
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

	if (m_queued_jobs == MAX_JOBS)
	{
		// Don't lock, overflow queue is exclusively owned by GS thread.
		m_overflow_job_queue.push_back(job);
		return;
	}

	std::lock_guard lock(m_mutex);

	// Push the new job and as many as possible from the overflow queue.
	while (job)
	{
		if (m_queued_jobs < MAX_JOBS)
		{
			m_job_queue[m_job_tail] = job;
			m_job_timer_queue[m_job_tail].Reset();

			m_job_tail = (m_job_tail + 1) % MAX_JOBS;
			m_queued_jobs++;

			m_worker_cv.notify_one();
		}

		if (!m_overflow_job_queue.empty())
		{
			job = m_overflow_job_queue.front();
			m_overflow_job_queue.pop_front();
		}
		else
		{
			job = nullptr;
		}
	}
}

void GSShaderCompilerAsync::WorkerThreadFunc(u32 thread_id)
{
	while (true)
	{
		// Acquire a queued job.
		GSCompileJob* job;
		{
			std::unique_lock lock(m_mutex);

			m_worker_cv.wait(lock, [&]() {
				return m_workers_stop || m_acquired_jobs < m_queued_jobs;
			});

			if (m_workers_stop)
				return;

			job = m_job_queue[m_job_acquire];

			m_job_acquire = (m_job_acquire + 1) % MAX_JOBS;
			m_acquired_jobs++;
		}

		// Compile the job.
		DoCompileJobSync(job, thread_id);

		// Release the completed job.
		job->SetThreadID(thread_id); // For debugging
		job->SetDoneCompiling();
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

	m_job_queue.clear(); // Release jobs references.
}

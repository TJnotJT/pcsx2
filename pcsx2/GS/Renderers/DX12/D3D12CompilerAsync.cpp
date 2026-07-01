#include "GS/Renderers/DX12/D3D12CompilerAsync.h"


D3D12CompilerAsync::D3D12CompilerAsync(
	D3D::ShaderModel shader_model, bool debug, u32 num_threads, u32 check_latency_ms)
	: m_shader_model(shader_model)
	, m_debug(debug)
	, m_check_latency_ms(check_latency_ms)
{
	m_worker_threads.resize(num_threads);
}

bool D3D12CompilerAsync::IsJobQueueFull()
{
	std::unique_lock lock(m_mutex);

	return m_queued_jobs == MAX_JOBS;
}

u32 D3D12CompilerAsync::GetCompileResultsAsync(CompileJob* out, u32 max_count)
{
	if (static_cast<u32>(m_check_timer.GetTimeMilliseconds()) < m_check_latency_ms)
		return 0;
	m_check_timer.Reset();

	std::unique_lock lock(m_mutex);

	u32 i = 0;
	while (m_acquired_jobs > 0 && m_job_done[m_job_head] && i < max_count)
	{
		CompileJob result;

		out[i] = std::move(m_job_queue[m_job_head]);
		out[i].time_ms = static_cast<float>(m_job_timer_queue[m_job_head].GetTimeMilliseconds());
		i++;

		m_job_queue[m_job_head] = {};
		m_job_done[m_job_head] = false;

		m_job_head = (m_job_head + 1) % MAX_JOBS;

		pxAssert(m_acquired_jobs <= m_queued_jobs);
		m_queued_jobs--;
		m_acquired_jobs--;
	}

	return i;
}

void D3D12CompilerAsync::StartCompileJobAsync(CompileJob job)
{
	if (!m_workers_started)
	{
		m_workers_started = true;
		u32 thread_id = 0;
		for (std::thread& t : m_worker_threads)
			t = std::thread([&, id=thread_id++]() { DoWorker(id); });
	}

	if (!m_check_timer_init)
		m_check_timer.Reset();

	std::unique_lock lock(m_mutex);

	if (m_queued_jobs < MAX_JOBS)
	{
		m_job_queue[m_job_tail] = std::move(job);
		m_job_done[m_job_tail] = false;
		m_job_timer_queue[m_job_tail].Reset();

		m_job_tail = (m_job_tail + 1) % MAX_JOBS;
		m_queued_jobs++;

		m_worker_cv.notify_all();
	}
}

void D3D12CompilerAsync::DoCompileJobSync(D3D12CompilerAsync::CompileJob& job)
{
	if (!job.vs_job.blob)
	{
		job.vs_job.blob = D3D::CompileShader(D3D::ShaderType::Vertex, m_shader_model, m_debug,
			job.vs_job.shader_code, job.vs_job.macros.GetPtr(), job.vs_job.entry_point.c_str());
	}

	if (!job.ps_job.blob)
	{
		job.ps_job.blob = D3D::CompileShader(D3D::ShaderType::Pixel, m_shader_model, m_debug,
			job.ps_job.shader_code, job.ps_job.macros.GetPtr(), job.ps_job.entry_point.c_str());
	}

	if (!job.pipeline_job.pipeline && (job.vs_job.blob && job.ps_job.blob))
	{
		if (!job.pipeline_job.gpb.GetDesc().VS.pShaderBytecode)
			job.pipeline_job.gpb.SetVertexShader(job.vs_job.blob.get());

		if (!job.pipeline_job.gpb.GetDesc().PS.pShaderBytecode)
			job.pipeline_job.gpb.SetPixelShader(job.ps_job.blob.get());

		if (job.pipeline_job.type == PipelineJob::GRAPHICS &&
			job.pipeline_job.gpb.GetDesc().VS.pShaderBytecode &&
			job.pipeline_job.gpb.GetDesc().PS.pShaderBytecode)
		{
			job.pipeline_job.pipeline = job.pipeline_job.gpb.Create(job.pipeline_job.device, false);
		}
		else
		{
			pxAssert(false); // Not supported yet
		}
	}
}

void D3D12CompilerAsync::DoWorker(u32 thread_id)
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
		DoCompileJobSync(m_job_queue[curr]);
		m_job_queue[curr].thread_id = thread_id; // For debugging

		// Release the completed job.
		{
			std::unique_lock lock(m_mutex);

			m_job_done[curr] = true;
		}
	}
}

D3D12CompilerAsync::~D3D12CompilerAsync()
{
	{
		std::unique_lock lock(m_mutex);

		m_workers_stop = true;

		m_worker_cv.notify_all();
	}

	for (std::thread& t : m_worker_threads)
	{
		if (t.joinable())
			t.join();
	}

	m_job_queue = {};
	m_job_done = {};
}
#include "GS/Renderers/DX12/D3D12CompilerAsync.h"

bool D3D12CompilerAsync::HasCompileResultAsync()
{
	std::unique_lock lock(m_mutex);

	return m_done_jobs > 0;
}

bool D3D12CompilerAsync::IsJobQueueFull()
{
	std::unique_lock lock(m_mutex);

	return m_queued_jobs == MAX_JOBS;
}

u32 D3D12CompilerAsync::GetCompileResultsAsync(CompileJob* out, u32 max_count)
{
	std::unique_lock lock(m_mutex);

	u32 i = 0;
	while (m_done_jobs > 0 && i < max_count)
	{
		D3D12CompilerAsync::CompileJob result;

		out[i++] = std::move(m_job_queue[m_job_head]);

		m_job_queue[m_job_head] = {};

		m_job_head = (m_job_head + 1) % MAX_JOBS;
		m_queued_jobs--;
		m_done_jobs--;
	}

	return i;
}

void D3D12CompilerAsync::StartCompileJobAsync(CompileJob job)
{
	std::unique_lock lock(m_mutex);

	if (!m_worker_started)
	{
		m_worker_thread = std::thread([&]() { DoWorker(); });
	}

	if (m_queued_jobs < MAX_JOBS)
	{
		m_job_queue[m_job_tail] = job;

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
		job.ps_job.blob = D3D::CompileShader(D3D::ShaderType::Vertex, m_shader_model, m_debug,
			job.ps_job.shader_code, job.ps_job.macros.GetPtr(), job.ps_job.entry_point.c_str());
	}

	if (!job.pipeline_job.pipeline)
	{
		if (job.pipeline_job.type == PipelineJob::GRAPHICS)
		{
			job.pipeline_job.desc.VS.pShaderBytecode = const_cast<ID3DBlob*>(job.vs_job.blob.get())->GetBufferPointer();
			job.pipeline_job.desc.VS.BytecodeLength = const_cast<ID3DBlob*>(job.vs_job.blob.get())->GetBufferSize();

			job.pipeline_job.desc.PS.pShaderBytecode = const_cast<ID3DBlob*>(job.vs_job.blob.get())->GetBufferPointer();
			job.pipeline_job.desc.PS.BytecodeLength = const_cast<ID3DBlob*>(job.vs_job.blob.get())->GetBufferSize();

			job.pipeline_job.device->CreateGraphicsPipelineState(
				&job.pipeline_job.desc, IID_PPV_ARGS(job.pipeline_job.pipeline.put()));
		}
		else
		{
			pxAssert(false); // Not supported yet
		}
	}
}

void D3D12CompilerAsync::DoWorker()
{
	while (true)
	{
		u32 done = 0;
		{
			std::unique_lock lock(m_mutex);

			m_worker_cv.wait(lock, [&]() {
				return m_stop || m_queued_jobs > 0;
			});

			if (m_stop)
				return;

			done = m_job_done;
		}

		DoCompileJobSync(m_job_queue[done]);

		{
			std::unique_lock lock(m_mutex);

			m_job_done++;
			m_done_jobs++;
		}
	}
}

D3D12CompilerAsync::~D3D12CompilerAsync()
{
	{
		std::unique_lock lock(m_mutex);

		m_stop = true;

		m_worker_cv.notify_all();
	}

	if (m_worker_thread.joinable())
		m_worker_thread.join();

	m_job_queue = {};
}
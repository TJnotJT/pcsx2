#pragma once

#include "GS/Renderers/DX11/D3D.h"
#include "GS/Renderers/DX12/D3D12Builders.h"
#include <d3d12.h>

#include <thread>
#include <mutex>

class D3D12CompilerAsync
{
public:
	template<typename T>
	using ComPtr = wil::com_ptr_nothrow<T>;

	struct ShaderJob
	{
		D3D::ShaderCacheEntryType type;
		std::string shader_code;
		D3D::ShaderMacro macros;
		std::string entry_point;
		ComPtr<ID3DBlob> blob;
	};

	struct PipelineJob
	{
		ID3D12Device* device;
		enum { GRAPHICS, COMPUTE } type;
		D3D12::GraphicsPipelineBuilder gpb;
		ComPtr<ID3D12PipelineState> pipeline;
	};

	struct CompileJob
	{
		ShaderJob vs_job;
		ShaderJob ps_job;
		PipelineJob pipeline_job;
		u64 job_hash;
	};

	D3D12CompilerAsync(D3D::ShaderModel shader_model, bool debug)
		: m_shader_model(shader_model), m_debug(debug) {}
	~D3D12CompilerAsync();

	bool HasCompileResultAsync();
	bool IsJobQueueFull();
	u32 GetCompileResultsAsync(CompileJob* out, u32 max_count);
	void StartCompileJobAsync(CompileJob job);

	void DoCompileJobSync(CompileJob& job);

private:
	static constexpr int MAX_JOBS = 1024;

	std::array<CompileJob, MAX_JOBS> m_job_queue;
	u32 m_job_head = 0; // head <= done <= tail
	u32 m_job_done = 0;
	u32 m_job_tail = 0;
	u32 m_queued_jobs = 0; // Count from head to tail
	u32 m_done_jobs = 0; // Count from head to done

	D3D::ShaderModel m_shader_model = D3D::ShaderModel::SM51;
	bool m_debug = false;

	std::thread m_worker_thread;
	std::mutex m_mutex;
	std::condition_variable m_worker_cv;
	bool m_stop = false;
	bool m_worker_started = false;
	
	void DoWorker();
};

struct AsyncReturn
{
private:
	const bool enabled; // Enable async return.
	bool async_return; // Call returned after dispatching async processing.

public:
	AsyncReturn(bool enabled = true) : enabled(enabled), async_return(false) {}

	static bool Enabled(AsyncReturn* async)
	{
		return async && async->enabled;
	}

	static void ClearAsync(AsyncReturn* async)
	{
		if (async)
			async->async_return = false;
	}

	static void SetAsync(AsyncReturn* async)
	{
		if (async)
			async->async_return = true;
	}

	static bool IsAsync(AsyncReturn* async)
	{
		return async && async->enabled && async->async_return;
	}
};
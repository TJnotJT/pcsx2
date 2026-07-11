#pragma once

#include "common/Timer.h"

#include "GS/Renderers/DX11/D3D.h"
#include "GS/Renderers/DX12/D3D12Builders.h"
#include <d3d12.h>

#include <thread>
#include <mutex>
#include <variant>

class D3D12CompilerAsync
{
public:
	template<typename T>
	using ComPtr = wil::com_ptr_nothrow<T>;

	struct ShaderJob
	{
		// Inputs
		D3D::ShaderCacheEntryType type;
		std::string shader_code;
		D3D::ShaderMacro macros;
		std::string entry_point;
		u64 hash; // For debugging
		bool uber; // For debugging
		
		// Output
		ComPtr<ID3DBlob> blob;

		bool Matches(const ShaderJob& other) const;
	};

	struct PipelineJob
	{
		// Inputs
		ID3D12Device* device;
		D3D12::GraphicsPipelineBuilder gpb;
		ComPtr<ID3DBlob> vs_blob;
		ComPtr<ID3DBlob> ps_blob;
		u64 hash; // For debugging
		bool uber; // For debugging
		
		// Output
		ComPtr<ID3D12PipelineState> pipeline;
	};

	struct CompileJob
	{
		float time_ms; // Compile time in ms for debugging.
		u32 thread_id; // Thread ID for debugging.

		// Job info
		std::variant<ShaderJob, PipelineJob> job;
	};

	D3D12CompilerAsync(D3D::ShaderModel shader_model, bool debug, u32 num_threads = 1,
		u32 check_latency_ms = 20);
	~D3D12CompilerAsync();

	bool IsJobQueueFull();
	u32 GetCompileResultsAsync(CompileJob* out, u32 max_count);
	void StartCompileJobAsync(CompileJob job);
private:
	void DoCompileJobSync(CompileJob& job);

	static constexpr int MAX_JOBS = 1024;

	std::array<CompileJob, MAX_JOBS> m_job_queue;
	std::array<bool, MAX_JOBS> m_job_done{};
	std::array<Common::Timer, MAX_JOBS> m_job_timer_queue;
	u32 m_job_head = 0; // head <= acquire <= tail
	u32 m_job_acquire = 0;
	u32 m_job_tail = 0;
	u32 m_queued_jobs = 0; // Count from head to tail
	u32 m_acquired_jobs = 0; // Count from acquire to tail

	D3D::ShaderModel m_shader_model = D3D::ShaderModel::SM51;
	bool m_debug = false;

	Common::Timer m_check_timer;
	bool m_check_timer_init = false;
	u32 m_check_latency_ms = 20;

	std::vector<std::thread> m_worker_threads;
	std::mutex m_mutex;
	std::condition_variable m_worker_cv;
	bool m_workers_stop = false;
	bool m_workers_started = false;
	
	void DoWorker(u32 thread_id);
};

struct AsyncReturn
{
private:
	const bool enabled; // Enable async return.
	bool async_return; // Call returned after dispatching async processing.
public:
	AsyncReturn(bool enabled) : enabled(enabled), async_return(false) {}

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
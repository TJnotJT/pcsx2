#pragma once

#include "GS/GSShaderCompilerAsync.h"

#include "GS/Renderers/DX11/D3D.h"
#include "GS/Renderers/DX12/D3D12Builders.h"
#include <d3d12.h>

#include <thread>
#include <mutex>
#include <variant>

class D3D12ShaderCompilerAsync : public GSShaderCompilerAsync
{
public:
	template<typename T>
	using ComPtr = wil::com_ptr_nothrow<T>;

	struct D3D12ShaderJob
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

		bool Matches(const D3D12ShaderJob& other) const;
	};

	struct D3D12PipelineJob
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

	struct D3D12CompileJob : GSCompileJob
	{
		// Job info
		std::variant<D3D12ShaderJob, D3D12PipelineJob> job;

		D3D12CompileJob(const D3D12ShaderJob& shader_job) : job(shader_job) {}
		D3D12CompileJob(D3D12ShaderJob&& shader_job) : job(std::move(shader_job)) {}

		D3D12CompileJob(const D3D12PipelineJob& pipeline_job) : job(pipeline_job) {}
		D3D12CompileJob(D3D12PipelineJob&& pipeline_job) : job(std::move(pipeline_job)) {}
	};

	D3D12ShaderCompilerAsync(u32 num_threads, u32 check_latency_ms, D3D::ShaderModel shader_model, bool debug);

protected:
	void DoCompileJobSync(GSCompileJob* job) override;

private:
	D3D::ShaderModel m_shader_model;
	bool m_debug;
};

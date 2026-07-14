#include "GS/Renderers/DX12/D3D12ShaderCompilerAsync.h"

bool D3D12ShaderCompilerAsync::D3D12ShaderJob::Matches(const D3D12ShaderJob& other) const
{
	bool matches =
		type == other.type &&
		shader_code == other.shader_code &&
		macros == other.macros &&
		entry_point == other.entry_point;

	// Sanity check, make sure debugging fields are same also.
	pxAssert(!matches || ((hash == other.hash) && (uber == other.uber)));

	return matches;
}

D3D12ShaderCompilerAsync::D3D12ShaderCompilerAsync(
	u32 num_threads, u32 check_latency_ms, D3D::ShaderModel shader_model, bool debug)
	: GSShaderCompilerAsync(num_threads, check_latency_ms)
	, m_shader_model(shader_model)
	, m_debug(debug)
{
}

void D3D12ShaderCompilerAsync::DoCompileJobSync(GSCompileJob* job_)
{
	D3D12CompileJob* job = static_cast<D3D12CompileJob*>(job_);

	if (std::holds_alternative<D3D12ShaderJob>(job->job))
	{
		D3D12ShaderJob& shader_job = std::get<D3D12ShaderJob>(job->job);
		if (shader_job.type == D3D::ShaderCacheEntryType::VertexShader)
		{
			shader_job.blob = D3D::CompileShader(D3D::ShaderType::Vertex, m_shader_model, m_debug,
				shader_job.shader_code, shader_job.macros.GetPtr(), shader_job.entry_point.c_str());
		}
		else if (shader_job.type == D3D::ShaderCacheEntryType::PixelShader)
		{
			shader_job.blob = D3D::CompileShader(D3D::ShaderType::Pixel, m_shader_model, m_debug,
				shader_job.shader_code, shader_job.macros.GetPtr(), shader_job.entry_point.c_str());
		}
		else
		{
			pxFailRel("Unknown shader type");
		}
	}
	else if (std::holds_alternative<D3D12PipelineJob>(job->job))
	{
		D3D12PipelineJob& pipeline_job = std::get<D3D12PipelineJob>(job->job);
		pxAssert(pipeline_job.gpb.GetDesc().VS.pShaderBytecode &&
			pipeline_job.gpb.GetDesc().PS.pShaderBytecode);
		pipeline_job.pipeline = pipeline_job.gpb.Create(pipeline_job.device, false);
	}
	else
	{
		pxFailRel("Unknown job type");
	}
}

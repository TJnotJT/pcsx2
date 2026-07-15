#include "GS/Renderers/Vulkan/VKShaderCompilerAsync.h"
#include "GS/Renderers/Vulkan/GSDeviceVK.h"

#include "common/Assertions.h"

bool VKShaderCompilerAsync::VKShaderJob::Matches(const VKShaderJob& other) const
{
	const bool matches = (kind == other.kind && shader_code == other.shader_code);

	// Sanity check, make sure debugging fields are same also.
	pxAssert(!matches || ((hash == other.hash) && (uber == other.uber)));

	return matches;
}

VKShaderCompilerAsync::VKShaderCompilerAsync(u32 num_threads, u32 check_latency_ms)
	: GSShaderCompilerAsync(num_threads, check_latency_ms)
{
}

void VKShaderCompilerAsync::OnWorkersStarted()
{
	if (!VKDynamicShaderc::Open())
		return;

	m_shaderc_compilers.resize(GetNumThreads());
	for (shaderc_compiler_t& compiler : m_shaderc_compilers)
		compiler = VKDynamicShaderc::CreateCompiler();
}

void VKShaderCompilerAsync::DoCompileJobSync(GSCompileJob* job_, u32 thread_id)
{
	if (m_shaderc_compilers.empty())
		return;

	VKCompileJob* job = static_cast<VKCompileJob*>(job_);

	if (std::holds_alternative<VKShaderJob>(job->job))
	{
		shaderc_compiler_t compiler = m_shaderc_compilers[thread_id];

		VKShaderJob& shader_job = std::get<VKShaderJob>(job->job);
		std::optional<SPIRVCodeVector> spv;

		shader_job.module = VK_NULL_HANDLE;
		spv = VKDynamicShaderc::CompileShaderToSPV(compiler, shader_job.kind,
			shader_job.shader_code, GSConfig.UseDebugDevice,
			GSDeviceVK::GetInstance()->GetOptionalExtensions().vk_khr_shader_non_semantic_info);

		if (spv)
		{
			const VkShaderModuleCreateInfo ci{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spv->size() * sizeof(SPIRVCodeType), spv->data() };

			vkCreateShaderModule(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &shader_job.module);

			shader_job.spv = std::move(*spv);
		}
	}
	else if (std::holds_alternative<VKPipelineJob>(job->job))
	{
		VKPipelineJob& pipeline_job = std::get<VKPipelineJob>(job->job);
		pxAssert(pipeline_job.gpb.HasVertexShader() && pipeline_job.gpb.HasFragmentShader());
		pipeline_job.pipeline = pipeline_job.gpb.Create(pipeline_job.device, pipeline_job.pipeline_cache, false);
	}
	else
	{
		pxFailRel("Unknown job type");
	}
}

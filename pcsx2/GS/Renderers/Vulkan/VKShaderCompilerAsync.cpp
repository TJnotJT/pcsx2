#include "GS/Renderers/Vulkan/VKShaderCompilerAsync.h"
#include "GS/Renderers/Vulkan/GSDeviceVK.h"

#include "common/Assertions.h"

void VKShaderCompilerAsync::VKPipelineJob::Create()
{
	pxAssert(m_gpb.HasVertexShader() && m_gpb.HasFragmentShader());
	m_pipeline = m_gpb.Create(m_device, m_pipeline_cache, false);
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

void VKShaderCompilerAsync::DoCompileJobSync(GSCompileJob* job, u32 thread_id)
{
	if (m_shaderc_compilers.empty())
		return;

	if (job->IsShaderJob())
	{
		VKShaderJob* shader_job = static_cast<VKShaderJob*>(job);

		shaderc_compiler_t compiler = m_shaderc_compilers[thread_id];

		std::optional<SPIRVCodeVector> spv =
			VKDynamicShaderc::CompileShaderToSPV(compiler, shader_job->GetKind(),
				shader_job->GetShaderCode(), GSConfig.UseDebugDevice,
				GSDeviceVK::GetInstance()->GetOptionalExtensions().vk_khr_shader_non_semantic_info);

		if (spv)
		{
			const VkShaderModuleCreateInfo ci{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spv->size() * sizeof(SPIRVCodeType), spv->data() };

			VkShaderModule mod = VK_NULL_HANDLE;
			vkCreateShaderModule(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &mod);

			if (mod != VK_NULL_HANDLE)
			{
				shader_job->SetModule(mod);
				shader_job->SetSPV(std::move(*spv));
			}
		}
	}
	else if (job->IsPipelineJob())
	{
		VKPipelineJob* pipeline_job = static_cast<VKPipelineJob*>(job);
		pipeline_job->Create();
	}
	else
	{
		pxFailRel("Unknown job type");
	}
}

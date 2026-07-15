#pragma once

#include "GS/Renderers/Vulkan/VKBuilders.h"
#include "GS/Renderers/Vulkan/VKDynamicShaderc.h"
#include "GS/GSShaderCompilerAsync.h"

#include "shaderc/shaderc.h"

#include <thread>
#include <mutex>
#include <variant>

class VKShaderCompilerAsync : public GSShaderCompilerAsync
{
public:
	using SPIRVCodeType = VKDynamicShaderc::SPIRVCodeType;
	using SPIRVCodeVector = VKDynamicShaderc::SPIRVCodeVector;

	struct VKShaderJob
	{
		// Inputs
		shaderc_shader_kind kind;
		std::string shader_code;
		u64 hash; // For debugging
		bool uber; // For debugging
		
		// Output
		VkShaderModule module;
		SPIRVCodeVector spv;

		bool Matches(const VKShaderJob& other) const;
	};

	struct VKPipelineJob
	{
		// Inputs
		VkDevice device;
		VkPipelineCache pipeline_cache;
		Vulkan::GraphicsPipelineBuilder gpb;
		VkShaderModule vs_module;
		VkShaderModule fs_module;
		u64 uid; // UID for GSDeviceVK to identify. Not stable across PCSX2 executions.
		u64 hash; // For debugging
		bool uber; // For debugging
		
		// Output
		VkPipeline pipeline;
	};

	struct VKCompileJob : GSCompileJob
	{
		// Job info
		std::variant<VKShaderJob, VKPipelineJob> job;

		VKCompileJob(const VKShaderJob& shader_job) : job(shader_job) {}
		VKCompileJob(VKShaderJob&& shader_job) : job(std::move(shader_job)) {}

		VKCompileJob(const VKPipelineJob& pipeline_job) : job(pipeline_job) {}
		VKCompileJob(VKPipelineJob&& pipeline_job) : job(std::move(pipeline_job)) {}
	};

	VKShaderCompilerAsync(u32 num_threads, u32 check_latency_ms);

protected:
	void DoCompileJobSync(GSCompileJob* job, u32 thread_id) override;
	void OnWorkersStarted() override;

private:
	std::vector<shaderc_compiler_t> m_shaderc_compilers;
};

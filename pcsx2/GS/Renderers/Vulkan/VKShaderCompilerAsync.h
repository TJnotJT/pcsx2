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

	class VKShaderJob : public GSCompileJob
	{
	public:
		VKShaderJob(shaderc_shader_kind kind, std::string_view shader_code, u64 hash, bool uber)
			: GSCompileJob(SHADER), m_kind(kind), m_shader_code(shader_code), m_hash(hash), m_uber(uber)
		{
		}
		shaderc_shader_kind GetKind() const { return m_kind; }
		const std::string& GetShaderCode() const { return m_shader_code; }
		const u64 GetHash() const { return m_hash; }
		const bool IsUber() const { return m_uber; }
		void SetModule(VkShaderModule module) { m_module = module; }
		VkShaderModule GetModule() const { return m_module; }
		template<typename T>
		void SetSPV(T&& spv) { m_spv = std::forward<T>(spv); }
		const SPIRVCodeVector& GetSPV() const { return m_spv; }
	private:
		// Inputs
		shaderc_shader_kind m_kind;
		std::string m_shader_code;
		u64 m_hash;
		bool m_uber;
		
		// Output
		VkShaderModule m_module = VK_NULL_HANDLE;
		SPIRVCodeVector m_spv;
	};

	class VKPipelineJob : public GSCompileJob
	{
	public:
		VKPipelineJob(VkDevice device, VkPipelineCache pipeline_cache,
			const Vulkan::GraphicsPipelineBuilder& gpb, u64 hash, bool uber)
			: GSCompileJob(PIPELINE), m_device(device), m_pipeline_cache(pipeline_cache)
			, m_gpb(gpb), m_hash(hash), m_uber(uber)
		{
		}

		VkDevice GetDevice() const { return m_device; }
		VkPipelineCache GetPipelineCache() const { return m_pipeline_cache; }
		u64 GetHash() const { return m_hash; }
		bool IsUber() const { return m_uber; }

		bool HasVS() const { return m_gpb.HasVertexShader(); }
		bool HasFS() const { return m_gpb.HasFragmentShader(); }
		void SetVS(VkShaderModule vs) { m_gpb.SetVertexShader(vs); }
		void SetFS(VkShaderModule fs) { m_gpb.SetFragmentShader(fs); }

		VKShaderJob* GetVSJob() const { return m_vs_job.get(); }
		VKShaderJob* GetFSJob() const { return m_fs_job.get(); }
		void SetVSJob(std::shared_ptr<VKShaderJob> vs_job) { m_vs_job = std::move(vs_job); }
		void SetFSJob(std::shared_ptr<VKShaderJob> fs_job) { m_fs_job = std::move(fs_job); }

		void Create();

		VkPipeline GetPipeline() const { return m_pipeline; }
	private:
		// Inputs
		VkDevice m_device;
		VkPipelineCache m_pipeline_cache;
		Vulkan::GraphicsPipelineBuilder m_gpb;
		u64 m_hash;
		bool m_uber;
		
		// Optional - shader jobs this pipeline is waiting on.
		// Use shared_ptr to preserve lifetime until this job is finished.
		std::shared_ptr<VKShaderJob> m_vs_job;
		std::shared_ptr<VKShaderJob> m_fs_job;

		// Output
		VkPipeline m_pipeline = VK_NULL_HANDLE;
	};

	// Overloads for templated functions.
	static VkPipeline GetOutput(const VKPipelineJob& job) { return job.GetPipeline(); }
	static VkShaderModule GetOutput(const VKShaderJob& job) { return job.GetModule(); }

	VKShaderCompilerAsync(u32 num_threads, u32 check_latency_ms);

protected:
	void DoCompileJobSync(GSCompileJob* job, u32 thread_id) override;
	void OnWorkersStarted() override;

private:
	std::vector<shaderc_compiler_t> m_shaderc_compilers;
};

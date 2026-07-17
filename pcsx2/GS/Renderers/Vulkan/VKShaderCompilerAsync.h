#pragma once

#include "common/Assertions.h"

#include "GS/Renderers/Vulkan/VKBuilders.h"
#include "GS/Renderers/Vulkan/VKDynamicShaderc.h"
#include "GS/GSShaderCompilerAsync.h"

#include "shaderc/shaderc.h"

#include <thread>
#include <mutex>
#include <variant>

template<typename KeyType>
class VKShaderCompilerAsync : public GSShaderCompilerAsync
{
public:
	using SPIRVCodeType = VKDynamicShaderc::SPIRVCodeType;
	using SPIRVCodeVector = VKDynamicShaderc::SPIRVCodeVector;

	class VKShaderJob : public GSCompileJob
	{
	public:
		VKShaderJob(VkDevice device, shaderc_shader_kind kind, std::string_view shader_code, u64 hash, bool uber)
			: GSCompileJob(SHADER), m_device(device), m_kind(kind), m_shader_code(shader_code), m_hash(hash), m_uber(uber)
		{
		}
		VkDevice GetDevice() const { return m_device; }
		shaderc_shader_kind GetKind() const { return m_kind; }
		const std::string& GetShaderCode() const { return m_shader_code; }
		const u64 GetHash() const { return m_hash; }
		const bool IsUber() const { return m_uber; }
		void SetModule(VkShaderModule module) { m_module = module; }
		VkShaderModule GetModule() const { return m_module; }
		template<typename T>
		void SetSPV(T&& spv) { m_spv = std::forward<T>(spv); }
		const SPIRVCodeVector& GetSPV() const { return m_spv; }

		virtual const KeyType& GetCacheKey() const = 0;
	private:
		// Inputs
		VkDevice m_device;
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

		virtual const KeyType& GetCacheKey() const = 0;

		void Create()
		{
			pxAssert(m_gpb.HasVertexShader() && m_gpb.HasFragmentShader());
			m_pipeline = m_gpb.Create(m_device, m_pipeline_cache, false);
		}

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
	template<typename KeyType>
	static VkPipeline GetOutput(const VKPipelineJob& job) { return job.GetPipeline(); }
	template<typename KeyType>
	static VkShaderModule GetOutput(const VKShaderJob& job) { return job.GetModule(); }

	VKShaderCompilerAsync(u32 num_threads, u32 check_latency_ms, bool debug, bool non_semantic)
		: GSShaderCompilerAsync(num_threads, check_latency_ms)
		, m_debug(debug)
		, m_non_semantic(non_semantic)
	{
	}

protected:
	void DoCompileJobSync(GSCompileJob* job, u32 thread_id) override
	{
		if (m_shaderc_compilers.empty())
			return;

		if (job->IsShaderJob())
		{
			VKShaderJob* shader_job = static_cast<VKShaderJob*>(job);

			shaderc_compiler_t compiler = m_shaderc_compilers[thread_id];

			std::optional<SPIRVCodeVector> spv =
				VKDynamicShaderc::CompileShaderToSPV(compiler, shader_job->GetKind(),
					shader_job->GetShaderCode(), m_debug, m_debug && m_non_semantic);

			if (spv)
			{
				const VkShaderModuleCreateInfo ci{
					VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spv->size() * sizeof(SPIRVCodeType), spv->data() };

				VkShaderModule mod;
				vkCreateShaderModule(shader_job->GetDevice(), &ci, nullptr, &mod);

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

	void OnWorkersStarted() override
	{
		if (!VKDynamicShaderc::Open())
			return;

		m_shaderc_compilers.resize(GetNumThreads());
		for (shaderc_compiler_t& compiler : m_shaderc_compilers)
			compiler = VKDynamicShaderc::CreateCompiler();
	}

private:
	std::vector<shaderc_compiler_t> m_shaderc_compilers;
	bool m_debug;
	bool m_non_semantic;
};

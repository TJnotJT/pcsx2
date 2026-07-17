// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "Config.h"

#include "GS/Renderers/Vulkan/VKLoader.h"
#include "GS/Renderers/Vulkan/VKDynamicShaderc.h"
#include "GS/Renderers/Vulkan/VKShaderCompilerAsync.h"

#include "common/HashCombine.h"

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class VKShaderCache
{
public:
	struct CacheIndexKey
	{
		u64 source_hash_low;
		u64 source_hash_high;
		u32 source_length;
		u32 shader_type;

		bool operator==(const CacheIndexKey& key) const;
		bool operator!=(const CacheIndexKey& key) const;
	};

	static CacheIndexKey GetCacheKey(u32 type, const std::string_view shader_code);
	static CacheIndexKey GetGraphicsPipelineCacheKey(
		const CacheIndexKey& vs_key, const CacheIndexKey& fs_key, const VkGraphicsPipelineCreateInfo& ci);

	struct CacheIndexEntryHasher
	{
		std::size_t operator()(const CacheIndexKey& e) const noexcept
		{
			std::size_t h = 0;
			HashCombine(h, e.source_hash_low, e.source_hash_high, e.source_length, e.shader_type);
			return h;
		}
	};

	struct CacheIndexData
	{
		u32 file_offset;
		u32 blob_size;
	};

	using ShaderCompilerAsync = VKShaderCompilerAsync<CacheIndexKey>;
	using VKShaderJob = ShaderCompilerAsync::VKShaderJob;
	using VKPipelineJob = ShaderCompilerAsync::VKPipelineJob;
	using GSCompileJob = GSShaderCompilerAsync::GSCompileJob;

	struct VKCachedShaderModule
	{
		VkShaderModule module;
		VKShaderCache::CacheIndexKey key;
	};

	struct VKCachedPipeline
	{
		VkPipeline pipeline;
		VKShaderCache::CacheIndexKey key;
	};

	class VKCachedPipelineJob : public VKPipelineJob
	{
	public:
		VKCachedPipelineJob(VkDevice device, VkPipelineCache pipeline_cache,
			const Vulkan::GraphicsPipelineBuilder& gpb, u64 hash, bool uber,
			const CacheIndexKey& cache_key)
			: VKPipelineJob(device, pipeline_cache, gpb, hash, uber)
			, m_cache_key(cache_key)
		{
		}

		virtual const CacheIndexKey& GetCacheKey() const override { return m_cache_key; }
	private:
		CacheIndexKey m_cache_key;
	};

	class VKCachedShaderJob : public VKShaderJob
	{
	public:
		VKCachedShaderJob(VkDevice device, shaderc_shader_kind kind, std::string_view shader_code, u64 hash, bool uber)
			: VKShaderJob(device, kind, shader_code, hash, uber)
			, m_cache_key(VKShaderCache::GetCacheKey(kind, shader_code))
		{
		}

		virtual const CacheIndexKey& GetCacheKey() const override { return m_cache_key; }
	private:
		CacheIndexKey m_cache_key;
	};

	~VKShaderCache();

	static void Create();
	static void Destroy();

	/// Returns a handle to the pipeline cache. Set set_dirty to true if you are planning on writing to it externally.
	VkPipelineCache GetPipelineCache(bool set_dirty = true, bool uber = false);

	/// Writes pipeline cache to file, saving all newly compiled pipelines.
	bool FlushPipelineCache();

	bool HasVertexShader(std::string_view shader_code, bool uber);
	bool HasFragmentShader(std::string_view shader_code, bool uber);

	VKCachedShaderModule GetVertexShader(std::string_view shader_code, bool uber);
	VKCachedShaderModule GetFragmentShader(std::string_view shader_code, bool uber);
	VkShaderModule GetComputeShader(std::string_view shader_code);

	bool HasPipelineState(const VKCachedShaderModule& vs, const VKCachedShaderModule& fs,
		const VkGraphicsPipelineCreateInfo& ci, bool uber);
	VKCachedPipeline GetGraphicsPipeline(VkDevice device, const CacheIndexKey& vs_key, const CacheIndexKey& fs_key,
		const VkGraphicsPipelineCreateInfo& ci, bool uber);

	void StartPipelineCompilationAsync(std::shared_ptr<GSCompileJob> job);
	void ProcessAsyncCompileJobs(); // Process jobs that have finished.
private:
	// SPIR-V compiled code type
	using SPIRVCodeType = VKDynamicShaderc::SPIRVCodeType;
	using SPIRVCodeVector = VKDynamicShaderc::SPIRVCodeVector;

	using CacheIndex = std::unordered_map<CacheIndexKey, CacheIndexData, CacheIndexEntryHasher>;
	using CacheSet = std::unordered_set<CacheIndexKey, CacheIndexEntryHasher>;


	VKShaderCache();

	static std::string GetShaderCacheBaseFileName(bool uber, bool debug);
	static std::string GetPipelineCacheBaseFileName(bool uber, bool debug);

	void Open();

	bool CreateNewShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber);
	bool ReadExistingShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber);
	void CloseShaderCache();

	bool CreateNewPipelineCache(bool uber);
	bool ReadExistingPipelineCache(bool uber);
	void ClosePipelineCache();

	static std::optional<VKShaderCache::SPIRVCodeVector> CompileShaderToSPV(
		u32 stage, std::string_view source, bool debug);
	bool HasShaderSPV(u32 type, std::string_view shader_code, bool uber);
	std::optional<SPIRVCodeVector> GetShaderSPV(u32 type, std::string_view shader_code, bool uber);
	std::optional<SPIRVCodeVector> CompileAndAddShaderSPV(const CacheIndexKey& key, std::string_view shader_code, bool uber);
	VKCachedShaderModule GetShaderModule(u32 type, std::string_view shader_code, bool uber);
	void AddShaderSPV(u32 type, std::string_view shader_code, const SPIRVCodeVector& spv,
		bool uber, bool only_new);

	void AddPipelineKey(const CacheIndexKey& key, bool uber);

	static bool InitShadercCompiler();

	// Start pipeline jobs that are waiting on the given vertex and/or fragment shader.
	void StartQueuedPipelineJobs(const VKCachedShaderJob* shader_job);

	std::FILE* m_index_file = nullptr;
	std::FILE* m_blob_file = nullptr;
	std::string m_pipeline_cache_filename;
	std::string m_pipeline_cache_index_filename;
	std::FILE* m_pipeline_cache_index_file = nullptr;

	std::FILE* m_uber_index_file = nullptr;
	std::FILE* m_uber_blob_file = nullptr;
	std::string m_uber_pipeline_cache_filename;
	std::string m_uber_pipeline_cache_index_filename;
	std::FILE* m_uber_pipeline_cache_index_file = nullptr;

	CacheIndex m_index;
	CacheIndex m_uber_index;

	CacheSet m_pipeline_index;
	CacheSet m_uber_pipeline_index;
	std::vector<CacheIndexKey> m_new_pipeline_index;
	std::vector<CacheIndexKey> m_uber_new_pipeline_index;

	VkPipelineCache m_pipeline_cache = VK_NULL_HANDLE;
	bool m_pipeline_cache_dirty = false;

	VkPipelineCache m_uber_pipeline_cache = VK_NULL_HANDLE;
	bool m_uber_pipeline_cache_dirty = false;

	std::FILE*& GetIndexFile(bool uber) { return uber ? m_uber_index_file : m_index_file; }
	std::FILE*& GetBlobFile(bool uber) { return uber ? m_uber_blob_file : m_blob_file; }
	std::string& GetPipelineCacheIndexFilename(bool uber) { return uber ? m_uber_pipeline_cache_index_filename : m_pipeline_cache_index_filename; }
	std::string& GetPipelineCacheFilename(bool uber) { return uber ? m_uber_pipeline_cache_filename : m_pipeline_cache_filename; }
	CacheIndex& GetIndex(bool uber) { return uber ? m_uber_index : m_index; }
	CacheSet& GetPipelineIndex(bool uber) { return uber ? m_uber_pipeline_index : m_pipeline_index; }
	std::vector<CacheIndexKey>& GetPipelineNewIndex(bool uber) { return uber ? m_uber_new_pipeline_index : m_new_pipeline_index; }
	bool& GetPipelineCacheDirty(bool uber) { return uber ? m_uber_pipeline_cache_dirty : m_pipeline_cache_dirty; }
	VkPipelineCache& GetPipelineCachePrivate(bool uber) { return uber ? m_uber_pipeline_cache : m_pipeline_cache; }

	static shaderc_compiler_t m_compiler_sync;
	static bool m_shaderc_failed;

	std::unique_ptr<ShaderCompilerAsync> m_compiler_async;
	std::deque<std::shared_ptr<GSCompileJob>> m_compile_jobs_async;
	std::deque<VKCachedPipelineJob*> m_queued_pipeline_jobs_async;
};

extern std::unique_ptr<VKShaderCache> g_vulkan_shader_cache;

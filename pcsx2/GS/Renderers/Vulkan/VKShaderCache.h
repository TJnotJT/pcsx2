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
#include <vector>

class VKShaderCache
{
public:
	using VKShaderJob = VKShaderCompilerAsync::VKShaderJob;
	using VKPipelineJob = VKShaderCompilerAsync::VKPipelineJob;
	using GSCompileJob = GSShaderCompilerAsync::GSCompileJob;

	~VKShaderCache();

	static void Create();
	static void Destroy();

	/// Returns a handle to the pipeline cache. Set set_dirty to true if you are planning on writing to it externally.
	VkPipelineCache GetPipelineCache(bool set_dirty = true, bool uber = false);

	/// Writes pipeline cache to file, saving all newly compiled pipelines.
	bool FlushPipelineCache();

	bool HasVertexShader(std::string_view shader_code, bool uber);
	bool HasFragmentShader(std::string_view shader_code, bool uber);

	VkShaderModule GetVertexShader(std::string_view shader_code, bool uber);
	VkShaderModule GetFragmentShader(std::string_view shader_code, bool uber);
	VkShaderModule GetComputeShader(std::string_view shader_code);

	void StartPipelineCompilationAsync(std::shared_ptr<GSCompileJob> job);
	void ProcessAsyncCompileJobs(); // Process jobs that have finished.

private:
	// SPIR-V compiled code type
	using SPIRVCodeType = VKDynamicShaderc::SPIRVCodeType;
	using SPIRVCodeVector = VKDynamicShaderc::SPIRVCodeVector;

	struct CacheIndexKey
	{
		u64 source_hash_low;
		u64 source_hash_high;
		u32 source_length;
		u32 shader_type;

		bool operator==(const CacheIndexKey& key) const;
		bool operator!=(const CacheIndexKey& key) const;
	};

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

	using CacheIndex = std::unordered_map<CacheIndexKey, CacheIndexData, CacheIndexEntryHasher>;

	VKShaderCache();

	static std::string GetShaderCacheBaseFileName(bool uber, bool debug);
	static std::string GetPipelineCacheBaseFileName(bool uber, bool debug);
	static CacheIndexKey GetCacheKey(u32 type, const std::string_view shader_code);
	static std::optional<VKShaderCache::SPIRVCodeVector> CompileShaderToSPV(
		u32 stage, std::string_view source, bool debug);

	void Open();

	bool CreateNewShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber);
	bool ReadExistingShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber);
	void CloseShaderCache();

	bool CreateNewPipelineCache(bool uber);
	bool ReadExistingPipelineCache(bool uber);
	void ClosePipelineCache();

	bool HasShaderSPV(u32 type, std::string_view shader_code, bool uber);
	std::optional<SPIRVCodeVector> GetShaderSPV(u32 type, std::string_view shader_code, bool uber);
	std::optional<SPIRVCodeVector> CompileAndAddShaderSPV(const CacheIndexKey& key, std::string_view shader_code, bool uber);
	VkShaderModule GetShaderModule(u32 type, std::string_view shader_code, bool uber);
	void AddShaderSPV(u32 type, std::string_view shader_code, const SPIRVCodeVector& spv,
		bool uber, bool only_new);

	static bool InitShadercCompiler();

	// Start pipeline jobs that are waiting on the given vertex and/or fragment shader.
	void StartQueuedPipelineJobs(const VKShaderJob* shader_job);

	std::FILE* m_index_file = nullptr;
	std::FILE* m_blob_file = nullptr;
	std::string m_pipeline_cache_filename;

	std::FILE* m_uber_index_file = nullptr;
	std::FILE* m_uber_blob_file = nullptr;
	std::string m_uber_pipeline_cache_filename;

	CacheIndex m_index;
	CacheIndex m_uber_index;

	VkPipelineCache m_pipeline_cache = VK_NULL_HANDLE;
	bool m_pipeline_cache_dirty = false;

	VkPipelineCache m_uber_pipeline_cache = VK_NULL_HANDLE;
	bool m_uber_pipeline_cache_dirty = false;

	std::FILE*& GetIndexFile(bool uber) { return uber ? m_uber_index_file : m_index_file; }
	std::FILE*& GetBlobFile(bool uber) { return uber ? m_uber_blob_file : m_blob_file; }
	std::string& GetPipelineCacheFilename(bool uber) { return uber ? m_uber_pipeline_cache_filename : m_pipeline_cache_filename; }
	CacheIndex& GetIndex(bool uber) { return uber ? m_uber_index : m_index; }
	bool& GetPipelineCacheDirty(bool uber) { return uber ? m_uber_pipeline_cache_dirty : m_pipeline_cache_dirty; }
	VkPipelineCache& GetPipelineCachePrivate(bool uber) { return uber ? m_uber_pipeline_cache : m_pipeline_cache; }

	static shaderc_compiler_t m_compiler_sync;
	static bool m_shaderc_failed;

	std::unique_ptr<VKShaderCompilerAsync> m_compiler_async;
	std::deque<std::shared_ptr<GSCompileJob>> m_compile_jobs_async;
	std::deque<VKPipelineJob*> m_queued_pipeline_jobs_async;
};

extern std::unique_ptr<VKShaderCache> g_vulkan_shader_cache;

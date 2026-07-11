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
	using VKCompileJob = VKShaderCompilerAsync::VKCompileJob;

	~VKShaderCache();

	static void Create();
	static void Destroy();

	/// Returns a handle to the pipeline cache. Set set_dirty to true if you are planning on writing to it externally.
	VkPipelineCache GetPipelineCache(bool set_dirty = true, bool uber = false);

	/// Writes pipeline cache to file, saving all newly compiled pipelines.
	bool FlushPipelineCache();

	VkShaderModule GetVertexShader(std::string_view shader_code, bool uber, GSAsyncReturn* async = nullptr);
	VkShaderModule GetFragmentShader(std::string_view shader_code, bool uber, GSAsyncReturn* async = nullptr);
	VkShaderModule GetComputeShader(std::string_view shader_code);

	void StartPipelineCompilationAsync(VKShaderJob vs_job, bool start_vs,
		VKShaderJob fs_job, bool start_fs, VKPipelineJob pipeline_job);

	struct FinishedPipelineJob
	{
		u64 uid;
		VkPipeline pipeline;
	};

	std::optional<FinishedPipelineJob> GetAsyncCompiledPipeline();

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

	std::optional<SPIRVCodeVector> GetShaderSPV(u32 type, std::string_view shader_code, bool uber, GSAsyncReturn* async = nullptr);
	std::optional<SPIRVCodeVector> CompileAndAddShaderSPV(const CacheIndexKey& key, std::string_view shader_code, bool uber);
	VkShaderModule GetShaderModule(u32 type, std::string_view shader_code, bool uber, GSAsyncReturn* async = nullptr);
	void AddShaderSPV(u32 type, std::string_view shader_code, const SPIRVCodeVector& spv,
		bool uber, bool only_new);

	static bool InitShadercCompiler();

	void ProcessAsyncCompileJobs();

	struct QueuedPipelineJob
	{
		VKShaderJob vs_job; // Vertex shader job needed to start.
		VKShaderJob fs_job; // Pixel shader job needed to start.
		VKPipelineJob pipeline_job; // Pipeline jobs that needs to start.
	};

	// Pipelines waiting for vertex/pixel shaders to finish compiling.
	std::deque<QueuedPipelineJob> m_queued_pipeline_jobs;

	void StartQueuedPipelineJobs(const VKShaderJob& job);

	// Jobs that are finished and need to be read back by GSDeviceVK.
	std::deque<FinishedPipelineJob> m_finished_pipeline_jobs;

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
	std::deque<VKCompileJob> m_compile_jobs_async;
};

extern std::unique_ptr<VKShaderCache> g_vulkan_shader_cache;

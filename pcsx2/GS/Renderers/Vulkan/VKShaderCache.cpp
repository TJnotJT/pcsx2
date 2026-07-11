// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/GSShaderCompileIndicator.h"
#include "GS/GS.h"
#include "GS/Renderers/Vulkan/GSDeviceVK.h"
#include "GS/Renderers/Vulkan/VKBuilders.h"
#include "GS/Renderers/Vulkan/VKShaderCache.h"
#include "GS/Renderers/Vulkan/VKDynamicShaderc.h"

#include "Config.h"
#include "ShaderCacheVersion.h"

#include "common/Assertions.h"
#include "common/Console.h"
#include "common/DynamicLibrary.h"
#include "common/Error.h"
#include "common/FileSystem.h"
#include "common/MD5Digest.h"
#include "common/Path.h"

#include "fmt/format.h"
#include "shaderc/shaderc.h"

#include <cstring>
#include <memory>

// TODO: store the driver version and stuff in the shader header

std::unique_ptr<VKShaderCache> g_vulkan_shader_cache;

static u32 s_next_bad_shader_id = 0;

namespace
{
#pragma pack(push, 4)
	struct VK_PIPELINE_CACHE_HEADER
	{
		u32 header_length;
		u32 header_version;
		u32 vendor_id;
		u32 device_id;
		u8 uuid[VK_UUID_SIZE];
	};

	struct CacheIndexEntry
	{
		u64 source_hash_low;
		u64 source_hash_high;
		u32 source_length;
		u32 shader_type;
		u32 file_offset;
		u32 blob_size;
	};
#pragma pack(pop)
} // namespace

static bool ValidatePipelineCacheHeader(const VK_PIPELINE_CACHE_HEADER& header)
{
	if (header.header_length < sizeof(VK_PIPELINE_CACHE_HEADER))
	{
		Console.Error("Pipeline cache failed validation: Invalid header length");
		return false;
	}

	if (header.header_version != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
	{
		Console.Error("Pipeline cache failed validation: Invalid header version");
		return false;
	}

	if (header.vendor_id != GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID)
	{
		Console.Error("Pipeline cache failed validation: Incorrect vendor ID (file: 0x%X, device: 0x%X)",
			header.vendor_id, GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID);
		return false;
	}

	if (header.device_id != GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID)
	{
		Console.Error("Pipeline cache failed validation: Incorrect device ID (file: 0x%X, device: 0x%X)",
			header.device_id, GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID);
		return false;
	}

	if (std::memcmp(header.uuid, GSDeviceVK::GetInstance()->GetDeviceProperties().pipelineCacheUUID, VK_UUID_SIZE) != 0)
	{
		Console.Error("Pipeline cache failed validation: Incorrect UUID");
		return false;
	}

	return true;
}

static void FillPipelineCacheHeader(VK_PIPELINE_CACHE_HEADER* header)
{
	header->header_length = sizeof(VK_PIPELINE_CACHE_HEADER);
	header->header_version = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
	header->vendor_id = GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID;
	header->device_id = GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID;
	std::memcpy(header->uuid, GSDeviceVK::GetInstance()->GetDeviceProperties().pipelineCacheUUID, VK_UUID_SIZE);
}

static void DumpBadShader(std::string_view code, std::string_view errors)
{
	const std::string filename = Path::Combine(EmuFolders::Logs, fmt::format("pcsx2_bad_shader_{}.txt", ++s_next_bad_shader_id));
	auto fp = FileSystem::OpenManagedCFile(filename.c_str(), "wb");
	if (fp)
	{
		if (!code.empty())
			std::fwrite(code.data(), code.size(), 1, fp.get());
		std::fputs("\n\n**** ERRORS ****\n", fp.get());
		if (!errors.empty())
			std::fwrite(errors.data(), errors.size(), 1, fp.get());
	}
}

shaderc_compiler_t VKShaderCache::m_compiler_sync = {};
bool VKShaderCache::m_shaderc_failed = false;

bool VKShaderCache::InitShadercCompiler()
{
	if (!VKDynamicShaderc::Open())
		return false;

	if (m_shaderc_failed)
		return false;

	if (m_compiler_sync)
		return true;

	m_compiler_sync = VKDynamicShaderc::CreateCompiler();

	m_shaderc_failed = !m_compiler_sync;

	return m_compiler_sync;
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::CompileShaderToSPV(u32 stage, std::string_view source, bool debug)
{
	std::optional<VKShaderCache::SPIRVCodeVector> ret;
	if (!InitShadercCompiler())
		return ret;

	const GSShaderCompileIndicator::CompileTimer compile_timer;

	std::string errors;

	ret = VKDynamicShaderc::CompileShaderToSPV(m_compiler_sync, stage, source, debug,
		GSDeviceVK::GetInstance()->GetOptionalExtensions().vk_khr_shader_non_semantic_info, &errors);

	if (!ret)
		DumpBadShader(source, errors);

	return ret;
}

VKShaderCache::VKShaderCache() = default;

VKShaderCache::~VKShaderCache()
{
	CloseShaderCache();
	FlushPipelineCache();
	ClosePipelineCache();
}

bool VKShaderCache::CacheIndexKey::operator==(const CacheIndexKey& key) const
{
	return (source_hash_low == key.source_hash_low && source_hash_high == key.source_hash_high &&
			source_length == key.source_length && shader_type == key.shader_type);
}

bool VKShaderCache::CacheIndexKey::operator!=(const CacheIndexKey& key) const
{
	return (source_hash_low != key.source_hash_low || source_hash_high != key.source_hash_high ||
			source_length != key.source_length || shader_type != key.shader_type);
}

void VKShaderCache::Create()
{
	pxAssert(!g_vulkan_shader_cache);
	g_vulkan_shader_cache.reset(new VKShaderCache());
	g_vulkan_shader_cache->Open();
}

void VKShaderCache::Destroy()
{
	g_vulkan_shader_cache.reset();
}

void VKShaderCache::Open()
{
	if (GSConfig.ShaderCacheType != GSShaderCacheType::Disabled)
	{
		for (u32 uber = 0; uber < 2; uber++)
		{
			GetPipelineCacheFilename(uber) = GetPipelineCacheBaseFileName(uber, GSConfig.UseDebugDevice);

			const std::string base_filename = GetShaderCacheBaseFileName(uber, GSConfig.UseDebugDevice);
			const std::string index_filename = base_filename + ".idx";
			const std::string blob_filename = base_filename + ".bin";

			if (!ReadExistingShaderCache(index_filename, blob_filename, uber))
				CreateNewShaderCache(index_filename, blob_filename, uber);

			if (!ReadExistingPipelineCache(uber))
				CreateNewPipelineCache(uber);
		}
	}
	else
	{
		for (u32 uber = 0; uber < 2; uber++)
			CreateNewPipelineCache(uber);
	}
}

VkPipelineCache VKShaderCache::GetPipelineCache(bool set_dirty /*= true*/, bool uber /*= false*/)
{
	if (GetPipelineCachePrivate(uber) == VK_NULL_HANDLE)
		return GetPipelineCachePrivate(uber);

	GetPipelineCacheDirty(uber) |= set_dirty;
	return GetPipelineCachePrivate(uber);
}

bool VKShaderCache::CreateNewShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber)
{
	if (FileSystem::FileExists(index_filename.c_str()))
	{
		Console.Warning("Removing existing index file '%s'", index_filename.c_str());
		FileSystem::DeleteFilePath(index_filename.c_str());
	}
	if (FileSystem::FileExists(blob_filename.c_str()))
	{
		Console.Warning("Removing existing blob file '%s'", blob_filename.c_str());
		FileSystem::DeleteFilePath(blob_filename.c_str());
	}

	GetIndexFile(uber) = FileSystem::OpenCFile(index_filename.c_str(), "wb");
	if (!GetIndexFile(uber))
	{
		Console.Error("Failed to open index file '%s' for writing", index_filename.c_str());
		return false;
	}

	const u32 file_version = SHADER_CACHE_VERSION;
	VK_PIPELINE_CACHE_HEADER header;
	FillPipelineCacheHeader(&header);

	if (std::fwrite(&file_version, sizeof(file_version), 1, GetIndexFile(uber)) != 1 ||
		std::fwrite(&header, sizeof(header), 1, GetIndexFile(uber)) != 1)
	{
		Console.Error("Failed to write header to index file '%s'", index_filename.c_str());
		std::fclose(GetIndexFile(uber));
		GetIndexFile(uber) = nullptr;
		FileSystem::DeleteFilePath(index_filename.c_str());
		return false;
	}

	GetBlobFile(uber) = FileSystem::OpenCFile(blob_filename.c_str(), "w+b");
	if (!GetBlobFile(uber))
	{
		Console.Error("Failed to open blob file '%s' for writing", blob_filename.c_str());
		std::fclose(GetIndexFile(uber));
		GetIndexFile(uber) = nullptr;
		FileSystem::DeleteFilePath(index_filename.c_str());
		return false;
	}

	return true;
}

bool VKShaderCache::ReadExistingShaderCache(const std::string& index_filename, const std::string& blob_filename, bool uber)
{
	GetIndexFile(uber) = FileSystem::OpenCFile(index_filename.c_str(), "r+b");
	if (!GetIndexFile(uber))
	{
		// special case here: when there's a sharing violation (i.e. two instances running),
		// we don't want to blow away the cache. so just continue without a cache.
		if (errno == EACCES)
		{
			Console.WriteLn("Failed to open shader cache index with EACCES, are you running two instances?");
			return true;
		}

		return false;
	}

	u32 file_version = 0;
	if (std::fread(&file_version, sizeof(file_version), 1, GetIndexFile(uber)) != 1 || file_version != SHADER_CACHE_VERSION)
	{
		Console.Error("Bad file/data version in '%s'", index_filename.c_str());
		std::fclose(GetIndexFile(uber));
		GetIndexFile(uber) = nullptr;
		return false;
	}

	VK_PIPELINE_CACHE_HEADER header;
	if (std::fread(&header, sizeof(header), 1, GetIndexFile(uber)) != 1 || !ValidatePipelineCacheHeader(header))
	{
		Console.Error("Mismatched pipeline cache header in '%s' (GPU/driver changed?)", index_filename.c_str());
		std::fclose(GetIndexFile(uber));
		GetIndexFile(uber) = nullptr;
		return false;
	}

	GetBlobFile(uber) = FileSystem::OpenCFile(blob_filename.c_str(), "a+b");
	if (!GetBlobFile(uber))
	{
		Console.Error("Blob file '%s' is missing", blob_filename.c_str());
		std::fclose(GetIndexFile(uber));
		GetIndexFile(uber) = nullptr;
		return false;
	}

	std::fseek(GetBlobFile(uber), 0, SEEK_END);
	const u32 blob_file_size = static_cast<u32>(std::ftell(GetBlobFile(uber)));

	for (;;)
	{
		CacheIndexEntry entry;
		if (std::fread(&entry, sizeof(entry), 1, GetIndexFile(uber)) != 1 ||
			(entry.file_offset + entry.blob_size) > blob_file_size)
		{
			if (std::feof(GetIndexFile(uber)))
				break;

			Console.Error("Failed to read entry from '%s', corrupt file?", index_filename.c_str());
			GetIndex(uber).clear();
			std::fclose(GetBlobFile(uber));
			GetBlobFile(uber) = nullptr;
			std::fclose(GetIndexFile(uber));
			GetIndexFile(uber) = nullptr;
			return false;
		}

		const CacheIndexKey key{entry.source_hash_low, entry.source_hash_high, entry.source_length, entry.shader_type};
		const CacheIndexData data{entry.file_offset, entry.blob_size};
		GetIndex(uber).emplace(key, data);
	}

	// ensure we don't write before seeking
	std::fseek(GetIndexFile(uber), 0, SEEK_END);

	Console.WriteLn("Read %zu entries from '%s'", GetIndex(uber).size(), index_filename.c_str());
	return true;
}

void VKShaderCache::CloseShaderCache()
{
	for (u32 uber = 0; uber < 2; uber++)
	{
		if (GetIndexFile(uber))
		{
			std::fclose(GetIndexFile(uber));
			GetIndexFile(uber) = nullptr;
		}
		if (GetBlobFile(uber))
		{
			std::fclose(GetBlobFile(uber));
			GetBlobFile(uber) = nullptr;
		}
	}
}

bool VKShaderCache::CreateNewPipelineCache(bool uber)
{
	if (!GetPipelineCacheFilename(uber).empty() && FileSystem::FileExists(GetPipelineCacheFilename(uber).c_str()))
	{
		Console.Warning("Removing existing pipeline cache '%s'", GetPipelineCacheFilename(uber).c_str());
		FileSystem::DeleteFilePath(GetPipelineCacheFilename(uber).c_str());
	}

	const VkPipelineCacheCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, 0, nullptr};
	VkResult res = vkCreatePipelineCache(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &GetPipelineCachePrivate(uber));
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreatePipelineCache() failed: ");
		return false;
	}

	GetPipelineCacheDirty(uber) = true;
	return true;
}

bool VKShaderCache::ReadExistingPipelineCache(bool uber)
{
	std::optional<std::vector<u8>> data = FileSystem::ReadBinaryFile(GetPipelineCacheFilename(uber).c_str());
	if (!data.has_value())
		return false;

	if (data->size() < sizeof(VK_PIPELINE_CACHE_HEADER))
	{
		Console.Error("Pipeline cache at '%s' is too small", GetPipelineCacheFilename(uber).c_str());
		return false;
	}

	VK_PIPELINE_CACHE_HEADER header;
	std::memcpy(&header, data->data(), sizeof(header));
	if (!ValidatePipelineCacheHeader(header))
		return false;

	const VkPipelineCacheCreateInfo ci{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, data->size(), data->data()};
	VkResult res = vkCreatePipelineCache(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &GetPipelineCachePrivate(uber));
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreatePipelineCache() failed: ");
		return false;
	}

	return true;
}

bool VKShaderCache::FlushPipelineCache()
{
	for (u32 uber = 0; uber < 2; uber++)
	{
		if (GetPipelineCachePrivate(uber) == VK_NULL_HANDLE || !GetPipelineCacheDirty(uber) || GetPipelineCacheFilename(uber).empty())
			return false;

		size_t data_size;
		VkResult res =
			vkGetPipelineCacheData(GSDeviceVK::GetInstance()->GetDevice(), GetPipelineCachePrivate(uber), &data_size, nullptr);
		if (res != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(res, "vkGetPipelineCacheData() failed: ");
			return false;
		}

		std::vector<u8> data(data_size);
		res = vkGetPipelineCacheData(GSDeviceVK::GetInstance()->GetDevice(), GetPipelineCachePrivate(uber), &data_size, data.data());
		if (res != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(res, "vkGetPipelineCacheData() (2) failed: ");
			return false;
		}

		data.resize(data_size);

		// Save disk writes if it hasn't changed, think of the poor SSDs.
		FILESYSTEM_STAT_DATA sd;
		if (!FileSystem::StatFile(GetPipelineCacheFilename(uber).c_str(), &sd) || sd.Size != static_cast<s64>(data_size))
		{
			Console.WriteLn("Writing %zu bytes to '%s'", data_size, GetPipelineCacheFilename(uber).c_str());
			if (!FileSystem::WriteBinaryFile(GetPipelineCacheFilename(uber).c_str(), data.data(), data.size()))
			{
				Console.Error("Failed to write pipeline cache to '%s'", GetPipelineCacheFilename(uber).c_str());
				return false;
			}
		}
		else
		{
			Console.WriteLn("Skipping updating pipeline cache '%s' due to no changes.", GetPipelineCacheFilename(uber).c_str());
		}

		GetPipelineCacheDirty(uber) = false;
	}
	return true;
}

void VKShaderCache::ClosePipelineCache()
{
	for (u32 uber = 0; uber < 2; uber++)
	{
		if (GetPipelineCachePrivate(uber) == VK_NULL_HANDLE)
			return;

		vkDestroyPipelineCache(GSDeviceVK::GetInstance()->GetDevice(), GetPipelineCachePrivate(uber), nullptr);
		GetPipelineCachePrivate(uber) = VK_NULL_HANDLE;
	}
}

std::string VKShaderCache::GetShaderCacheBaseFileName(bool uber, bool debug)
{
	std::string base_filename = "vulkan_shaders";

	if (uber)
		base_filename += "_uber";

	if (debug)
		base_filename += "_debug";

	return Path::Combine(EmuFolders::Cache, base_filename);
}

std::string VKShaderCache::GetPipelineCacheBaseFileName(bool uber, bool debug)
{
	std::string base_filename = "vulkan_pipelines";

	if (uber)
		base_filename += "_uber";

	if (debug)
		base_filename += "_debug";

	base_filename += ".bin";

	return Path::Combine(EmuFolders::Cache, base_filename);
}

VKShaderCache::CacheIndexKey VKShaderCache::GetCacheKey(u32 type, const std::string_view shader_code)
{
	union HashParts
	{
		struct
		{
			u64 hash_low;
			u64 hash_high;
		};
		u8 hash[16];
	};
	HashParts h;

	MD5Digest digest;
	digest.Update(shader_code.data(), static_cast<u32>(shader_code.length()));
	digest.Final(h.hash);

	return CacheIndexKey{h.hash_low, h.hash_high, static_cast<u32>(shader_code.length()), type};
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::GetShaderSPV(u32 type, std::string_view shader_code,
	bool uber, GSAsyncReturn* async)
{
	const auto key = GetCacheKey(type, shader_code);
	auto iter = GetIndex(uber).find(key);
	if (iter == GetIndex(uber).end())
	{
		if (GSAsyncReturn::Enabled(async))
		{
			// Don't start async compilation here yet, just flag that the shader is not yet compiled.
			GSAsyncReturn::SetAsync(async);
			return std::nullopt;
		}

		return CompileAndAddShaderSPV(key, shader_code, uber);
	}

	std::optional<SPIRVCodeVector> spv = SPIRVCodeVector(iter->second.blob_size);

	if (std::fseek(GetBlobFile(uber), iter->second.file_offset, SEEK_SET) != 0 ||
		std::fread(spv->data(), sizeof(SPIRVCodeType), iter->second.blob_size, GetBlobFile(uber)) != iter->second.blob_size)
	{
		Console.Error("Read blob from file failed, recompiling");
		spv = CompileShaderToSPV(type, shader_code, GSConfig.UseDebugDevice);
	}

	return spv;
}

VkShaderModule VKShaderCache::GetShaderModule(u32 type, std::string_view shader_code, bool uber, GSAsyncReturn* async)
{
	std::optional<SPIRVCodeVector> spv = GetShaderSPV(type, shader_code, uber, async);
	if (!spv.has_value())
		return VK_NULL_HANDLE;

	const VkShaderModuleCreateInfo ci{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spv->size() * sizeof(SPIRVCodeType), spv->data()};

	VkShaderModule mod;
	VkResult res = vkCreateShaderModule(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &mod);
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreateShaderModule() failed: ");
		return VK_NULL_HANDLE;
	}

	return mod;
}

VkShaderModule VKShaderCache::GetVertexShader(std::string_view shader_code, bool uber, GSAsyncReturn* async)
{
	ProcessAsyncCompileJobs();
	return GetShaderModule(shaderc_glsl_vertex_shader, std::move(shader_code), uber, async);
}

VkShaderModule VKShaderCache::GetFragmentShader(std::string_view shader_code, bool uber, GSAsyncReturn* async)
{
	ProcessAsyncCompileJobs();
	return GetShaderModule(shaderc_glsl_fragment_shader, std::move(shader_code), uber, async);
}

VkShaderModule VKShaderCache::GetComputeShader(std::string_view shader_code)
{
	ProcessAsyncCompileJobs();
	return GetShaderModule(shaderc_glsl_compute_shader, std::move(shader_code), false);
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::CompileAndAddShaderSPV(
	const CacheIndexKey& key, std::string_view shader_code, bool uber)
{
	std::optional<SPIRVCodeVector> spv = CompileShaderToSPV(key.shader_type, shader_code, GSConfig.UseDebugDevice);
	if (!spv.has_value())
		return {};

	if (!GetBlobFile(uber) || std::fseek(GetBlobFile(uber), 0, SEEK_END) != 0)
		return spv;

	CacheIndexData data;
	data.file_offset = static_cast<u32>(std::ftell(GetBlobFile(uber)));
	data.blob_size = static_cast<u32>(spv->size());

	CacheIndexEntry entry = {};
	entry.source_hash_low = key.source_hash_low;
	entry.source_hash_high = key.source_hash_high;
	entry.source_length = key.source_length;
	entry.shader_type = static_cast<u32>(key.shader_type);
	entry.blob_size = data.blob_size;
	entry.file_offset = data.file_offset;

	if (std::fwrite(spv->data(), sizeof(SPIRVCodeType), entry.blob_size, GetBlobFile(uber)) != entry.blob_size ||
		std::fflush(GetBlobFile(uber)) != 0 || std::fwrite(&entry, sizeof(entry), 1, GetIndexFile(uber)) != 1 ||
		std::fflush(GetIndexFile(uber)) != 0)
	{
		Console.Error("Failed to write shader blob to file");
		return spv;
	}

	GetIndex(uber).emplace(key, data);
	return spv;
}

void VKShaderCache::AddShaderSPV(u32 type, std::string_view shader_code, const SPIRVCodeVector& spv,
	bool uber, bool only_new)
{
	// FIXME: Duplication with CompileAndAddShaderSPV();

	const auto key = GetCacheKey(type, shader_code);

	if (only_new && GetIndex(uber).contains(key))
		return;

	if (!GetBlobFile(uber) || std::fseek(GetBlobFile(uber), 0, SEEK_END) != 0)
		return;

	CacheIndexData data;
	data.file_offset = static_cast<u32>(std::ftell(GetBlobFile(uber)));
	data.blob_size = static_cast<u32>(spv.size());

	CacheIndexEntry entry = {};
	entry.source_hash_low = key.source_hash_low;
	entry.source_hash_high = key.source_hash_high;
	entry.source_length = key.source_length;
	entry.shader_type = static_cast<u32>(key.shader_type);
	entry.blob_size = data.blob_size;
	entry.file_offset = data.file_offset;

	if (std::fwrite(spv.data(), sizeof(SPIRVCodeType), entry.blob_size, GetBlobFile(uber)) != entry.blob_size ||
		std::fflush(GetBlobFile(uber)) != 0 || std::fwrite(&entry, sizeof(entry), 1, GetIndexFile(uber)) != 1 ||
		std::fflush(GetIndexFile(uber)) != 0)
	{
		Console.Error("Failed to write shader blob to file");
	}

	GetIndex(uber).emplace(key, data);
}

void VKShaderCache::ProcessAsyncCompileJobs()
{
	if (m_compiler_async)
	{
		const u32 n_jobs = m_compiler_async->GetCompletedJobs();

		if (n_jobs > 0)
			Console.WriteLn("Async pipeline compile: processing %d jobs", n_jobs);

		for (u32 i = 0; i < n_jobs; i++)
		{
			VKCompileJob& compile_job = m_compile_jobs_async.front();

			// The async compiler completes jobs in FIFO order so the first n_jobs entries must be done.
			pxAssert(compile_job.done);

			if (std::holds_alternative<VKShaderJob>(compile_job.job))
			{
				VKShaderJob& shader_job = std::get<VKShaderJob>(compile_job.job);
				AddShaderSPV(shader_job.kind, shader_job.shader_code, shader_job.spv, shader_job.uber, true);

				pxAssert(shader_job.kind == shaderc_vertex_shader || shader_job.kind == shaderc_fragment_shader);
				const char* kind_str = (shader_job.kind == shaderc_vertex_shader) ? "vertex" : "fragment";
				Console.WriteLn("Async %s shader compile: finished hash=0x%016llX uber=%d time=%.2fms thread_id=%d",
					kind_str, shader_job.hash, shader_job.uber, compile_job.compile_time_ms, compile_job.thread_id);

				// Notify any pipelines waiting on this shader.
				StartQueuedPipelineJobs(shader_job);
			}
			else if (std::holds_alternative<VKPipelineJob>(compile_job.job))
			{
				VKPipelineJob& pipeline_job = std::get<VKPipelineJob>(compile_job.job);
				Console.WriteLn("Async pipeline compile: finished hash=0x%016llX uber=%d time=%.2fms thread_id=%d",
					pipeline_job.hash, pipeline_job.uber, compile_job.compile_time_ms, compile_job.thread_id);

				FinishedPipelineJob finished_job;
				finished_job.uid = pipeline_job.uid;
				finished_job.pipeline = pipeline_job.pipeline;
				
				m_finished_pipeline_jobs.push_back(finished_job);
			}
			else
			{
				pxFailRel("Unknown job type");
			}

			m_compile_jobs_async.pop_front();
		}
	}
}

void VKShaderCache::StartPipelineCompilationAsync(VKShaderJob vs_job, bool start_vs,
	VKShaderJob fs_job, bool start_fs, VKPipelineJob pipeline_job)
{
	if (!m_compiler_async)
		m_compiler_async = std::make_unique<VKShaderCompilerAsync>(
			GSConfig.HybridShaderCacheThreads, GSConfig.HybridShaderCacheLatencyMS);

	// If the job queue is full we may drop jobs since we don't allow resubmitting the same pipeline
	// for compilation twice.
	pxAssert(!m_compiler_async->IsJobQueueFull());

	// VS
	if (start_vs)
	{
		Console.WriteLn("Async vertex shader compile: started hash=0x%016llX uber=%d", vs_job.hash, vs_job.uber);
		m_compile_jobs_async.emplace_back(vs_job);
		m_compiler_async->StartCompileJobAsync(&m_compile_jobs_async.back());
	}

	// FS
	if (start_fs)
	{
		Console.WriteLn("Async fragment shader compile: started hash=0x%016llX uber=%d", fs_job.hash, fs_job.uber);
		m_compile_jobs_async.emplace_back(fs_job);
		m_compiler_async->StartCompileJobAsync(&m_compile_jobs_async.back());
	}

	// Pipeline
	if (pipeline_job.vs_module && pipeline_job.fs_module)
	{
		Console.WriteLn("Async pipeline compile: started hash=0x%016llX uber=%d", pipeline_job.hash, pipeline_job.uber);
		m_compile_jobs_async.emplace_back(std::move(pipeline_job));
		m_compiler_async->StartCompileJobAsync(&m_compile_jobs_async.back());
	}
	else
	{
		// Need to wait for vertex and/or pixel shader.
		Console.WriteLn("Async pipeline compile: queued hash=0x%016llX uber=%d vs_hash=0x%016llX fs_hash=0x%016llX",
			pipeline_job.hash, pipeline_job.uber, vs_job.hash, fs_job.hash);
		QueuedPipelineJob queued_job{ std::move(vs_job), std::move(fs_job), std::move(pipeline_job) };
		m_queued_pipeline_jobs.push_back(std::move(queued_job));
	}
}

void VKShaderCache::StartQueuedPipelineJobs(const VKShaderJob& shader_job)
{
	for (auto it = m_queued_pipeline_jobs.begin(); it != m_queued_pipeline_jobs.end(); )
	{
		QueuedPipelineJob& queued_job = *it;
		if (shader_job.kind == shaderc_vertex_shader)
		{
			if (!queued_job.pipeline_job.vs_module && queued_job.vs_job.Matches(shader_job))
			{
				queued_job.pipeline_job.vs_module = shader_job.module;
				queued_job.pipeline_job.gpb.SetVertexShader(shader_job.module);
			}
		}
		else if (shader_job.kind == shaderc_fragment_shader)
		{
			if (!queued_job.pipeline_job.fs_module && queued_job.fs_job.Matches(shader_job))
			{
				queued_job.pipeline_job.fs_module = shader_job.module;
				queued_job.pipeline_job.gpb.SetFragmentShader(shader_job.module);
			}
		}
		else
		{
			pxFailRel("Unknown shader type");
		}

		if (queued_job.pipeline_job.vs_module && queued_job.pipeline_job.fs_module)
		{
			// Vertex and pixel shaders compiled so start pipeline creating.
			Console.WriteLn("Async pipeline compile: got vs=%016X and ps=%016X for pipeline=%016X uber=%d",
				queued_job.vs_job.hash, queued_job.fs_job.hash, queued_job.pipeline_job.hash, queued_job.pipeline_job.uber);
			StartPipelineCompilationAsync({}, false, {}, false, std::move(queued_job.pipeline_job));
			it = m_queued_pipeline_jobs.erase(it);
		}
		else
		{
			it++;
		}
	}
}

std::optional<VKShaderCache::FinishedPipelineJob> VKShaderCache::GetAsyncCompiledPipeline()
{
	ProcessAsyncCompileJobs();
	if (!m_finished_pipeline_jobs.empty())
	{
		FinishedPipelineJob job = m_finished_pipeline_jobs.front();
		m_finished_pipeline_jobs.pop_front();
		return job;
	}
	return std::nullopt;
}
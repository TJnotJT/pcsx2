// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS/Renderers/Common/GSTexture.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GS/GSPng.h"
#include "GS/GSPerfMon.h"
#include "GS/GSGL.h"

#include "common/Console.h"
#include "common/BitUtils.h"
#include "common/StringUtil.h"

#include <bit>
#include <bitset>
#include <array>

GSTexture::GSTexture() = default;

GSTexture::~GSTexture() = default;

bool GSTexture::Save(const std::string& fn)
{
	// Depth textures need special treatment - we have a stencil component.
	// Just re-use the existing conversion shader instead.
	if (m_format == Format::DepthStencil || m_format == Format::Float32)
	{
		GSTexture* temp = g_gs_device->CreateRenderTarget(GetWidth(), GetHeight(), Format::Color, false);
		if (!temp)
		{
			Console.Error("Failed to allocate %dx%d texture for depth conversion", GetWidth(), GetHeight());
			return false;
		}

		g_gs_device->StretchRect(this, GSVector4::cxpr(0.0f, 0.0f, 1.0f, 1.0f), temp, GSVector4(GetRect()), ShaderConvert::FLOAT32_TO_RGBA8, false);
		const bool res = temp->Save(fn);
		g_gs_device->Recycle(temp);
		return res;
	}

	GSPng::Format format = (IsDevBuild || GSConfig.SaveAlpha) ? GSPng::RGB_A_PNG : GSPng::RGB_PNG;

	switch (m_format)
	{
		case Format::UNorm8:
			format = GSPng::R8I_PNG;
			break;
		case Format::Color:
			break;
		default:
			Console.Error("Format %d not saved to image", static_cast<int>(m_format));
			return false;
	}

	const GSVector4i rc(0, 0, m_size.x, m_size.y);
	std::unique_ptr<GSDownloadTexture> dl(g_gs_device->CreateDownloadTexture(m_size.x, m_size.y, m_format));
	if (!dl || (dl->CopyFromTexture(rc, this, rc, 0), dl->Flush(), !dl->Map(rc)))
	{
		Console.Error("(GSTexture) DownloadTexture() failed.");
		return false;
	}

	const int compression = GSConfig.PNGCompressionLevel;
	return GSPng::Save(format, fn, dl->GetMapPointer(), m_size.x, m_size.y, dl->GetMapPitch(), compression, false);
}

const char* GSTexture::GetFormatName(Format format)
{
	switch (format)
	{
		default:
			pxFailRel("Invalid texture format");
		case Format::Invalid:      return "Invalid";
		case Format::Color:        return "Color";
		case Format::ColorHQ:      return "ColorHQ";
		case Format::ColorHDR:     return "ColorHDR";
		case Format::ColorClip:    return "ColorClip";
		case Format::DepthStencil: return "DepthStencil";
		case Format::Float32:      return "Float32";
		case Format::UNorm8:       return "UNorm8";
		case Format::UInt16:       return "UInt16";
		case Format::UInt32:       return "UInt32";
		case Format::PrimID:       return "PrimID";
		case Format::BC1:          return "BC1";
		case Format::BC2:          return "BC2";
		case Format::BC3:          return "BC3";
		case Format::BC7:          return "BC7";
	}
}

bool GSTexture::IsBlockCompressedFormat(Format format)
{
	switch (format)
	{
		case Format::BC1:
		case Format::BC2:
		case Format::BC3:
		case Format::BC7:
			return true;
		default:
			return false;
	}
}

u32 GSTexture::GetCompressedBytesPerBlock() const
{
	return GetCompressedBytesPerBlock(m_format);
}

u32 GSTexture::GetCompressedBytesPerBlock(Format format)
{
	switch (format)
	{
		default:
			pxFailRel("Invalid texture format");
		case Format::Invalid:      return 1;  // Invalid
		case Format::Color:        return 4;  // Color/RGBA8
		case Format::ColorHQ:      return 4;  // ColorHQ/RGB10A2
		case Format::ColorHDR:     return 8;  // ColorHDR/RGBA16F
		case Format::ColorClip:    return 8;  // ColorClip/RGBA16
		case Format::DepthStencil: return 4;  // DepthStencil
		case Format::Float32:      return 4;  // Float32/R32
		case Format::UNorm8:       return 1;  // UNorm8/R8
		case Format::UInt16:       return 2;  // UInt16/R16UI
		case Format::UInt32:       return 4;  // UInt32/R32UI
		case Format::PrimID:       return 4;  // Int32/R32I
		case Format::BC1:          return 8;  // BC1 - 16 pixels in 64 bits
		case Format::BC2:          return 16; // BC2 - 16 pixels in 128 bits
		case Format::BC3:          return 16; // BC3 - 16 pixels in 128 bits
		case Format::BC7:          return 16; // BC7 - 16 pixels in 128 bits
	}
}

u32 GSTexture::GetCompressedBlockSize() const
{
	return GetCompressedBlockSize(m_format);
}

u32 GSTexture::GetCompressedBlockSize(Format format)
{
	return IsBlockCompressedFormat(format) ? 4 : 1;
}

u32 GSTexture::CalcUploadPitch(Format format, u32 width)
{
	if (format >= Format::BC1 && format <= Format::BC7)
		width = Common::AlignUpPow2(width, 4) / 4;

	return width * GetCompressedBytesPerBlock(format);
}

u32 GSTexture::CalcUploadPitch(u32 width) const
{
	return CalcUploadPitch(m_format, width);
}

u32 GSTexture::CalcUploadRowLengthFromPitch(u32 pitch) const
{
	return CalcUploadRowLengthFromPitch(m_format, pitch);
}

u32 GSTexture::CalcUploadRowLengthFromPitch(Format format, u32 pitch)
{
	const u32 block_size = GetCompressedBlockSize(format);
	const u32 bytes_per_block = GetCompressedBytesPerBlock(format);
	return ((pitch + (bytes_per_block - 1)) / bytes_per_block) * block_size;
}

u32 GSTexture::CalcUploadSize(u32 height, u32 pitch) const
{
	return CalcUploadSize(m_format, height, pitch);
}

u32 GSTexture::CalcUploadSize(Format format, u32 height, u32 pitch)
{
	const u32 block_size = GetCompressedBlockSize(format);
	return pitch * ((static_cast<u32>(height) + (block_size - 1)) / block_size);
}

void GSTexture::GenerateMipmapsIfNeeded()
{
	if (!m_needs_mipmaps_generated || m_mipmap_levels <= 1 || IsCompressedFormat())
		return;

	m_needs_mipmaps_generated = false;
	GenerateMipmap();
}

void GSTexture::CreateDepthColor()
{
	pxAssert(IsDepthStencil());

	if (!m_depth_color)
	{
		m_depth_color.reset(g_gs_device->CreateRenderTarget(GetWidth(), GetHeight(), Format::Float32, false));
		m_depth_color_valid_area = GSVector4i::zero();
#ifdef PCSX2_DEVBUILD
		if (GSConfig.UseDebugDevice)
		{
			// FIXME: Track the actual debug names.
			m_depth_color->SetDebugName(fmt::format("0x{:x} Depth color for @ 0x{:x}",
				reinterpret_cast<u64>(m_depth_color.get()), reinterpret_cast<u64>(this)));
		}
#endif
	}
}

void GSTexture::UpdateDepthColor(GSVector4i draw_area)
{
	pxAssert(IsDepthStencil());

	if (!m_depth_color)
		CreateDepthColor();

	GL_PUSH("HW: UpdateDepthColor {%d, %d, %d, %d}", draw_area.x, draw_area.y, draw_area.z, draw_area.w);

	// Align copied areas to 128 to avoid too many small copies.
	draw_area = draw_area.ralign<Align_Outside>(GSVector2i(128, 128)).rintersect(GetRect());

	// This is a bit hacky but we need to deactivate depth color in the body of this function
	// so that the shader copies use the depth resource instead of depth color as the source texture.
	m_depth_color_active = false;

	if (m_depth_color_valid_area.rcontains(draw_area) || // Everything is already valid.
		GetState() == State::Cleared) // Simply propagate clears.
	{
		GL_INS("HW: Cleared - early exit");
		m_depth_color_active = true;
		return;
	}

	if (m_depth_color_valid_area.rempty())
	{
		// No current valid area so just copy the whole draw area.
		GL_INS("HW: Initialize empty valid area.");

		GSVector4 dst_rect(draw_area);
		GSVector4 src_rect(dst_rect / GSVector4(GetSize()).xyxy());

		g_gs_device->StretchRect(this, src_rect, m_depth_color.get(), dst_rect, ShaderConvert::FLOAT32_DEPTH_TO_COLOR, false);

		m_depth_color_valid_area = draw_area;
		m_depth_color_active = true;
		return;
	}

	GL_INS("HW: Split new area into pieces");

	const GSVector4i new_valid_area_int = m_depth_color_valid_area.runion(draw_area);

	const GSVector4 old_valid_area(m_depth_color_valid_area);
	const GSVector4 new_valid_area(new_valid_area_int);

	const int less = (new_valid_area < old_valid_area).mask();
	const int greater = (new_valid_area > old_valid_area).mask();

	const bool need_left_area = (less & 1) != 0;
	const bool need_top_area = (less & 2) != 0;
	const bool need_right_area = (greater & 4) != 0;
	const bool need_bottom_area = (greater & 8) != 0;

	std::array<GSDevice::MultiStretchRect, 4> new_areas;
	u32 n_new_areas = 0;

	if (need_left_area)
	{
		// Whole left side of new area.
		new_areas[n_new_areas++].dst_rect = new_valid_area.insert32<0, 2>(old_valid_area);
	}

	if (need_right_area)
	{
		// Whole right side of new area.
		new_areas[n_new_areas++].dst_rect = new_valid_area.insert32<2, 0>(old_valid_area);
	}

	if (need_top_area)
	{
		// Top side of new area except right/left sides.
		new_areas[n_new_areas++].dst_rect = old_valid_area.xyzy().insert32<1, 1>(new_valid_area);
	}

	if (need_bottom_area)
	{
		// Bottom side of new area except right/left sides.
		new_areas[n_new_areas++].dst_rect = old_valid_area.xwzw().insert32<3, 3>(new_valid_area);
	}

	const GSVector4 dim = GSVector4(GetSize()).xyxy();
	for (u32 i = 0; i < n_new_areas; i++)
	{
		new_areas[i].src_rect = new_areas[i].dst_rect / dim;
		new_areas[i].linear = false;
		new_areas[i].src = this;
	}

	g_gs_device->DrawMultiStretchRects(new_areas.data(), n_new_areas, m_depth_color.get(),
	                                   ShaderConvert::FLOAT32_DEPTH_TO_COLOR);

	m_depth_color_active = true;
	m_depth_color_valid_area = new_valid_area_int;
}

void GSTexture::ResolveDepthColor()
{
	pxAssert(IsDepthStencil() && IsDepthColor());

	GL_PUSH("HW: ResolveDepthColor {%d, %d, %d, %d}",
		m_depth_color_valid_area.x, m_depth_color_valid_area.y,
		m_depth_color_valid_area.z, m_depth_color_valid_area.w);

	m_depth_color_active = false;
	m_depth_color_valid_area = GSVector4i::zero();

	// Simply propagate clears.
	if (GetState() == State::Cleared)
	{
		GL_INS("HW: Cleared - early exit");
		return;
	}

	GSVector4 dst_rect(0.0f, 0.0f, static_cast<float>(GetWidth()), static_cast<float>(GetHeight()));
	g_gs_device->StretchRect(m_depth_color.get(), this, dst_rect, ShaderConvert::FLOAT32_COLOR_TO_DEPTH, false);
}

GSDownloadTexture::GSDownloadTexture(u32 width, u32 height, GSTexture::Format format)
	: m_width(width)
	, m_height(height)
	, m_format(format)
{
}

GSDownloadTexture::~GSDownloadTexture() = default;

u32 GSDownloadTexture::GetBufferSize(u32 width, u32 height, GSTexture::Format format, u32 pitch_align /* = 1 */)
{
	const u32 block_size = GSTexture::GetCompressedBlockSize(format);
	const u32 bytes_per_block = GSTexture::GetCompressedBytesPerBlock(format);
	const u32 bw = (width + (block_size - 1)) / block_size;
	const u32 bh = (height + (block_size - 1)) / block_size;

	pxAssert(std::has_single_bit(pitch_align));
	const u32 pitch = Common::AlignUpPow2(bw * bytes_per_block, pitch_align);
	return (pitch * bh);
}

u32 GSDownloadTexture::GetTransferPitch(u32 width, u32 pitch_align) const
{
	const u32 block_size = GSTexture::GetCompressedBlockSize(m_format);
	const u32 bytes_per_block = GSTexture::GetCompressedBytesPerBlock(m_format);
	const u32 bw = (width + (block_size - 1)) / block_size;

	pxAssert(std::has_single_bit(pitch_align));
	return Common::AlignUpPow2(bw * bytes_per_block, pitch_align);
}

void GSDownloadTexture::GetTransferSize(const GSVector4i& rc, u32* copy_offset, u32* copy_size, u32* copy_rows) const
{
	const u32 block_size = GSTexture::GetCompressedBlockSize(m_format);
	const u32 bytes_per_block = GSTexture::GetCompressedBytesPerBlock(m_format);
	const u32 tw = static_cast<u32>(rc.width());
	const u32 tb = ((tw + (block_size - 1)) / block_size);

	*copy_offset = (((static_cast<u32>(rc.y) + (block_size - 1)) / block_size) * m_current_pitch) +
				   ((static_cast<u32>(rc.x) + (block_size - 1)) / block_size) * bytes_per_block;
	*copy_size = tb * bytes_per_block;
	*copy_rows = ((static_cast<u32>(rc.height()) + (block_size - 1)) / block_size);
}

bool GSDownloadTexture::ReadTexels(const GSVector4i& rc, void* out_ptr, u32 out_stride)
{
	if (m_needs_flush)
		Flush();

	if (!Map(rc))
		return false;

	const u32 block_size = GSTexture::GetCompressedBlockSize(m_format);
	const u32 bytes_per_block = GSTexture::GetCompressedBytesPerBlock(m_format);
	const u32 tw = static_cast<u32>(rc.width());
	const u32 tb = ((tw + (block_size - 1)) / block_size);

	const u32 copy_offset = (((static_cast<u32>(rc.y) + (block_size - 1)) / block_size) * m_current_pitch) +
							((static_cast<u32>(rc.x) + (block_size - 1)) / block_size) * bytes_per_block;
	const u32 copy_size = tb * bytes_per_block;
	const u32 copy_rows = ((static_cast<u32>(rc.height()) + (block_size - 1)) / block_size);

	StringUtil::StrideMemCpy(out_ptr, out_stride, m_map_pointer + copy_offset, m_current_pitch, copy_size, copy_rows);
	return true;
}

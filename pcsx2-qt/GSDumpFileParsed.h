#pragma once

#include "common/Pcsx2Defs.h"
#include "common/Pcsx2Types.h"

#include "GS/GSState.h"
#include "GS/GSDump.h"
#include "GS/GSGL.h"
#include "GS/GSPerfMon.h"
#include "GS/GSUtil.h"

#include "common/Console.h"
#include "common/BitUtils.h"
#include "common/Path.h"
#include "common/StringUtil.h"

#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <memory>

struct GSDumpFileParsed
{
	struct GSReg // GS register high-level structure
	{
		u32 m_reg_id;
		std::map<std::string, u64> m_fields;

		std::string getName() const;
		bool hasField(const std::string& field_name) const;
		u64 getField(const std::string& field_name) const;
	};

	struct GSDumpPacket
	{
		virtual GSDumpTypes::GSType getType();
	};

	struct GSDumpPrivRegs : GSDumpPacket
	{
		GSPrivRegSet m_regs;

		GSDumpPrivRegs()
		{
			memset(this, 0, sizeof(*this));
		}
		GSDumpPrivRegs(const u8* data);

		virtual GSDumpTypes::GSType getType()
		{
			return GSDumpTypes::GSType::Registers;
		}
	};

	struct GSDumpGIFPacket
	{
		GIFTag m_giftag;
		std::vector<GSReg> m_regs; // For PACKED and REGLIST
		std::vector<u8> m_image;   // For IMAGE

		static u32 GetDataSize(const GIFTag& tag);

		GSDumpGIFPacket(const u8* data, u32 max_size);

		bool GetReg(u32 index, GSReg* reg);
		u32 GetDataSize()
		{
			return GetDataSize(m_giftag);
		}
		u32 GetSize()
		{
			return sizeof(m_giftag) + GetDataSize();
		}
		u32 GetNumRegs()
		{
			assert(m_giftag.FLG == GIF_FLG_PACKED || m_giftag.FLG == GIF_FLG_REGLIST);
			return m_giftag.NREG * m_giftag.NLOOP;
		}
		u32 GetRegId(u32 i)
		{
			assert(i < m_giftag.NREG);
			return (m_giftag.REGS >> (4 * i)) & 0xF;
		}
	};

	struct GSDumpTransfer : GSDumpPacket
	{
		GSDumpTypes::GSTransferPath m_path;
		std::vector<std::unique_ptr<GSDumpGIFPacket>> m_packets;

		GSDumpTypes::GSType getType()
		{
			return GSDumpTypes::GSType::Transfer;
		}

		GSDumpTransfer(GSDumpTypes::GSTransferPath path, const u8* data, u32 size);
	};

	struct GSDumpVSync : GSDumpPacket
	{
		u8 field;
		GSDumpTypes::GSType getType()
		{
			return GSDumpTypes::GSType::VSync;
		}
		GSDumpVSync(const u8* data)
		{
			field = *data;
		}
	};
	
	std::string m_serial;
	u32 m_crc = 0;
	GSDrawingEnvironment m_env;
	GSVertex m_v;
	struct GSTransferBuffer
	{
		int x = 0, y = 0;
		int w = 0, h = 0;
		int start = 0, end = 0, total = 0;
		GSVector4i rect = GSVector4i::zero();
		GIFRegBITBLTBUF m_blit = {};
		GIFRegTRXPOS m_pos = {};
		GIFRegTRXREG m_reg = {};
		bool write = false;
	} m_tr;
	std::vector<u8> m_mem;
	GIFPath m_path[4] = {};
	float m_q = 1.0f;
	std::vector<GSDumpPacket> m_packets;
	GSDumpPrivRegs m_priv_regs;

	void ReadState(const u8* data, u32 size);

	GSDumpFileParsed(const GSDumpFile* dump_file);
};
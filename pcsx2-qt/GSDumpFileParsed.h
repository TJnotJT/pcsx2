#pragma once

#include "GSDumpEditorDefs.h"

#include <QStandardItemModel>

struct GSDumpFileParsed
{
	struct GSAbstractReg
	{
		u32 m_reg_id;
		std::map<std::string, u64> m_fields;

		virtual std::string GetName() const = 0;
		virtual u32 GetSize() const = 0;

		std::string ToString() const;
		bool HasField(const std::string& field_name) const;
		u64 GetField(const std::string& field_name) const;
		void AddToModel(QStandardItem* parent) const;

		GSAbstractReg(u32 reg_id)
			: m_reg_id(reg_id)
			, m_fields()
		{
		}
	};

	struct GSReg : GSAbstractReg
	{
		std::string GetName() const;
		u32 GetSize() const
		{
			return 8;
		};
		GSReg(u32 reg_id, const u8* data);
	};

	struct GSPackedReg : GSAbstractReg
	{
		std::string GetName() const;
		u32 GetSize() const
		{
			return 16;
		}
		GSPackedReg(u32 reg_id, const u8* data);
	};

	struct GSDumpPacket
	{
		virtual GSDumpTypes::GSType getType() = 0;
		virtual void AddToModel(QStandardItemModel* model) = 0;
	};

	struct GSDumpPrivRegs : GSDumpPacket
	{
		GSPrivRegSet m_regs;

		GSDumpPrivRegs()
		{
			memset(this, 0, sizeof(*this));
		}
		GSDumpPrivRegs(const u8* data);
		void AddToModel(QStandardItemModel* model);
		GSDumpTypes::GSType getType()
		{
			return GSDumpTypes::GSType::Registers;
		}
	};

	struct GSDumpGIFPacket
	{
		GIFTag m_giftag;
		std::vector<std::unique_ptr<GSAbstractReg>> m_regs; // For PACKED and REGLIST
		std::vector<u8> m_image;  // For IMAGE

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
		void AddToModel(QStandardItem* item);
	};

	struct GSDumpTransfer : GSDumpPacket
	{
		GSDumpTypes::GSTransferPath m_path;
		std::vector<std::unique_ptr<GSDumpGIFPacket>> m_packets;

		GSDumpTypes::GSType getType()
		{
			return GSDumpTypes::GSType::Transfer;
		}

		void AddToModel(QStandardItemModel* model);

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
		void AddToModel(QStandardItemModel* model)
		{
			model->appendRow(new QStandardItem("VSync"));
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
	std::vector<std::unique_ptr<GSDumpPacket>> m_packets;
	GSDumpPrivRegs m_priv_regs;

	void ReadState(const u8* data, u32 size);

	GSDumpFileParsed(const GSDumpFile* dump_file);

	// FIXME: Capitalize
	void AddToModel(QStandardItemModel* model);
};
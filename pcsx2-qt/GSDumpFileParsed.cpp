#include "GSDumpEditorDefs.h"
#include "GSDumpFileParsed.h"
#include "GSRegParseFormat.h"

#include <QStandardItemModel>

bool GSDumpFileParsed::GSAbstractReg::HasField(const std::string& field_name) const
{
	return m_fields.find(field_name) != m_fields.end();
}

u64 GSDumpFileParsed::GSAbstractReg::GetField(const std::string& field_name) const
{
	auto it = m_fields.find(field_name);
	if (it != m_fields.end())
	{
		return it->second;
	}
	Console.Error("GSDumpFileParsed::GSReg: Field '{}' not found in register '{}'.", field_name, GetName());
	return 0;
}

std::string GSDumpFileParsed::GSAbstractReg::ToString() const
{
	std::stringstream ss;
	ss << GetName();
	bool first = false;
	for (const auto& [name, value] : m_fields)
	{
		if (!first)
			ss << " ";
		ss << name << ":" << GSRegFormatField(); 
		first = false;
	}
	return ss.str();
}

void GSDumpFileParsed::GSAbstractReg::AddToModel(QStandardItem* parent) const
{
	QStandardItem* reg_item = new QStandardItem(QString::fromStdString(ToString()));
	parent->appendRow(reg_item);
}

GSDumpFileParsed::GSReg::GSReg(u32 reg_id, const u8* data)
	: GSAbstractReg(reg_id)
{
	GSRegDecodeFields(m_reg_id, (const u64*)data, &m_fields);
}

std::string GSDumpFileParsed::GSReg::GetName() const
{
	return GSRegGetName(m_reg_id);
}

GSDumpFileParsed::GSPackedReg::GSPackedReg(u32 reg_id, const u8* data)
	: GSAbstractReg(reg_id)
{
	GSPackedRegDecodeFields(m_reg_id, (const u64*)data, &m_fields);
}

std::string GSDumpFileParsed::GSPackedReg::GetName() const
{
	return GSPackedRegGetName(m_reg_id);
}

GSDumpFileParsed::GSDumpPrivRegs::GSDumpPrivRegs(const u8* data)
{
	memcpy(&m_regs, data, Ps2MemSize::GSregs);
}

void GSDumpFileParsed::GSDumpPrivRegs::AddToModel(QStandardItemModel* model)
{
	model->appendRow(new QStandardItem("PrivRegs"));
}

u32 GSDumpFileParsed::GSDumpGIFPacket::GetDataSize(const GIFTag& tag)
	{
	switch (tag.FLG)
	{
		case GIF_FLG_PACKED:
			return tag.NLOOP * tag.NREG * 16; // Each packed register is 16 bytes
		case GIF_FLG_REGLIST:
			return tag.NLOOP * tag.NREG * 8; // Each REGLIST register is 8 bytes
		case GIF_FLG_IMAGE2:
		case GIF_FLG_IMAGE:
			return tag.NLOOP * 16;
		default:
			Console.Error("GSDumpGIFPacket: Unknown GIF packet type.");
			assert(false);
			return 0;
	}
}

GSDumpFileParsed::GSDumpGIFPacket::GSDumpGIFPacket(const u8* data, u32 max_size)
{
	std::memcpy(&m_giftag, data, sizeof(GIFTag));

	if (max_size < sizeof(GIFTag) + GetDataSize(m_giftag))
	{
		Console.Error("GSDumpGIFPacket: Packet size too small for GIFTag(got {} bytes, expected {} bytes).",
			max_size, sizeof(GIFTag) + GetDataSize(m_giftag));
		assert(false);
		return;
	}

	const u8* data_orig = data;
	data += sizeof(GIFTag);

	m_regs = std::vector<std::unique_ptr<GSAbstractReg>>();

	switch (m_giftag.FLG)
	{
		case GIF_FLG_PACKED:
			for (size_t i = 0; i < m_giftag.NLOOP; i++)
			{
				for (size_t j = 0; j < m_giftag.NREG; j++)
				{
					m_regs.push_back(std::make_unique<GSPackedReg>(GetRegId(j), data));
					data += 16;
				}
			}
			break;
		case GIF_FLG_REGLIST:
			for (size_t i = 0; i < m_giftag.NLOOP; i++)
			{
				for (size_t j = 0; j < m_giftag.NREG; j++)
				{
					m_regs.push_back(std::make_unique<GSReg>(GetRegId(j), data));
					data += 8;
				}
			}
			break;
		case GIF_FLG_IMAGE2:
		case GIF_FLG_IMAGE: // image
			m_image = std::vector<u8>(GetDataSize());
			memcpy(m_image.data(), data, GetDataSize());
			data += GetDataSize();
			break;
		default:
			Console.Error("GSDumpGIFPacket: Bad FLG value encountered in packet ({})", m_giftag.FLG);
			break;
	}

	assert(data - data_orig == GetSize());
}

void GSDumpFileParsed::GSDumpGIFPacket::AddToModel(QStandardItem* parent)
{
	QStandardItem* gif_item = new QStandardItem("GifPacket");
	parent->appendRow(gif_item);
	for (const std::unique_ptr<GSAbstractReg>& reg : m_regs)
	{
		reg->AddToModel(gif_item);
	}
}

//bool GSDumpFileParsed::GSDumpGIFPacket::GetReg(u32 index, GSReg* reg)
//{
//	if (index >= giftag.NLOOP * giftag.NREG)
//	{
//		Console.Error("GSDumpGIFPacket: Register index out of bounds.");
//		return false;
//	}
//	if (giftag.FLG == GIF_FLG_PACKED || giftag.FLG == GIF_FLG_REGLIST)
//	{
//		u32 stride = giftag.FLG == GIF_FLG_PACKED ? 16 : 8; // PACKED registers are 16 bytes, REGLIST are 8 bytes
//		u32 offset = sizeof(GIFTag) + index * stride;
//		u32 reg_id = (giftag.REGS >> (index % giftag.NREG * 4)) & 0xF; // Extract reg_id from TAG
//		return giftag.FLG == GIF_FLG_PACKED ?
//					GSPackedRegDecodeFields(reg_id, (u64*)(data.data() + offset), &reg->field_vals) :
//					GSRegDecodeFields(reg_id, (u64*)(data.data() + offset), &reg->field_vals);
//	}
//	else if (giftag.FLG == GIF_FLG_IMAGE)
//	{
//		Console.Error("GSDumpGIFPacket: Cannot get register from IMAGE packet.");
//		return false;
//	}
//	else
//	{
//		Console.Error("GSDumpGIFPacket: Unknown GIF packet type.");
//		return false;
//	}
//}

GSDumpFileParsed::GSDumpTransfer::GSDumpTransfer(GSDumpTypes::GSTransferPath path, const u8* data, u32 size)
{
	m_path = path;
	u32 i = 0;
	while (i < size)
	{
		m_packets.push_back(std::make_unique<GSDumpGIFPacket>(data + i, size - i));
		i += m_packets[m_packets.size() - 1]->GetSize();
	}
	assert(i == size);
}

void GSDumpFileParsed::GSDumpTransfer::AddToModel(QStandardItemModel* model)
{
	QStandardItem* transfer_item = new QStandardItem("Transfer");
	model->appendRow(transfer_item);
	for (const std::unique_ptr<GSDumpGIFPacket>& gif_packet : m_packets)
	{
		gif_packet->AddToModel(transfer_item);
	}
}

// Copied from GSState::Defrost()
void GSDumpFileParsed::ReadState(const u8* data, u32 size)
{
	const u8* data_orig = data;

	auto ReadData = [&data](auto* a) {
		memcpy(a, data, sizeof(*a));
		data += sizeof(*a);
	};

	u32 version;
	ReadData(&version);
	if (version > 9) // FIXME: MOVE SAVE_STATE_VERSION FROM GSSTATE.H to GSDump.H
	{
		Console.Error("GS: Savestate version is incompatible.  Load aborted.");
		assert(false);
	}

	ReadData(&m_env.PRIM);

	if (version <= 6)
		data += sizeof(GIFRegPRMODE);

	ReadData(&m_env.PRMODECONT);
	ReadData(&m_env.TEXCLUT);
	ReadData(&m_env.SCANMSK);
	ReadData(&m_env.TEXA);
	ReadData(&m_env.FOGCOL);
	ReadData(&m_env.DIMX);
	ReadData(&m_env.DTHE);
	ReadData(&m_env.COLCLAMP);
	ReadData(&m_env.PABE);
	ReadData(&m_env.BITBLTBUF);
	ReadData(&m_env.TRXDIR);
	ReadData(&m_env.TRXPOS);
	ReadData(&m_env.TRXREG);
	ReadData(&m_env.TRXREG); // obsolete

	for (int i = 0; i < 2; i++)
	{
		ReadData(&m_env.CTXT[i].XYOFFSET);
		ReadData(&m_env.CTXT[i].TEX0);
		ReadData(&m_env.CTXT[i].TEX1);

		if (version <= 6)
			data += sizeof(GIFRegTEX2);

		ReadData(&m_env.CTXT[i].CLAMP);
		ReadData(&m_env.CTXT[i].MIPTBP1);
		ReadData(&m_env.CTXT[i].MIPTBP2);
		ReadData(&m_env.CTXT[i].SCISSOR);
		ReadData(&m_env.CTXT[i].ALPHA);
		ReadData(&m_env.CTXT[i].TEST);
		ReadData(&m_env.CTXT[i].FBA);
		ReadData(&m_env.CTXT[i].FRAME);
		ReadData(&m_env.CTXT[i].ZBUF);

		m_env.CTXT[i].XYOFFSET.OFX &= 0xffff;
		m_env.CTXT[i].XYOFFSET.OFY &= 0xffff;

		if (version <= 4)
			data += sizeof(u32) * 7; // skip
	}

	ReadData(&m_v.RGBAQ);
	ReadData(&m_v.ST);
	ReadData(&m_v.UV);
	ReadData(&m_v.FOG);
	ReadData(&m_v.XYZ);
	data += sizeof(GIFReg); // obsolete
	ReadData(&m_tr.x);
	ReadData(&m_tr.y);

	if (version >= 9)
	{
		ReadData(&m_tr.w);
		ReadData(&m_tr.h);
		ReadData(&m_tr.m_blit);
		ReadData(&m_tr.m_pos);
		ReadData(&m_tr.m_reg);
		ReadData(&m_tr.rect);
		ReadData(&m_tr.total);
		ReadData(&m_tr.start);
		ReadData(&m_tr.end);
		ReadData(&m_tr.write);
	}
	else
	{
		m_tr.w = m_env.TRXREG.RRW;
		m_tr.h = m_env.TRXREG.RRH;
		m_tr.m_blit = m_env.BITBLTBUF;
		m_tr.m_pos = m_env.TRXPOS;
		m_tr.m_reg = m_env.TRXREG;
		// Assume the last transfer was a write (but nuke it).
		m_tr.rect = GSVector4i(m_env.TRXPOS.DSAX, m_env.TRXPOS.DSAY, m_env.TRXPOS.DSAX + m_tr.w, m_env.TRXPOS.DSAY + m_tr.h);
		m_tr.total = 0;
		m_tr.start = 0;
		m_tr.end = 0;
		m_tr.write = true;
	}

	struct GSVMem
	{
		u8 data[VM_SIZE];
	};
	m_mem = std::vector<u8>(VM_SIZE);
	ReadData((GSVMem*)m_mem.data());

	for (GIFPath& path : m_path)
	{
		ReadData(&path.tag);
		ReadData(&path.reg);
		path.SetTag(&path.tag); // expand regs
	}

	ReadData(&m_q);

	assert(data - data_orig == size);
}

GSDumpFileParsed::GSDumpFileParsed(const GSDumpFile* dump_file)
{
	// Header stuff
	m_crc = dump_file->GetCRC();
	m_serial = dump_file->GetSerial();

	// Initial privilege regs
	assert(dump_file->GetRegsData().size() == Ps2MemSize::GSregs);
	m_priv_regs = GSDumpPrivRegs(dump_file->GetRegsData().data());

	// Initial state
	ReadState(dump_file->GetStateData().data(), dump_file->GetStateData().size());

	// Packets
	const std::vector<GSDumpFile::GSData>* packets = &dump_file->GetPackets();
	int i = 0;
	for (const GSDumpFile::GSData& packet : dump_file->GetPackets())
	{
		switch (packet.id)
		{
			case GSDumpTypes::GSType::Transfer:
				m_packets.push_back(std::make_unique<GSDumpTransfer>(packet.path, packet.data, packet.length));
				break;
			case GSDumpTypes::GSType::Registers:
				m_packets.push_back(std::make_unique<GSDumpPrivRegs>(packet.data));
				assert(packet.length == sizeof(GSPrivRegSet));
				break;
			case GSDumpTypes::GSType::VSync:
				m_packets.push_back(std::make_unique<GSDumpVSync>(packet.data));
				assert(packet.length == 1);
				break;
			default:
				Console.Error("GSDumpFileParsed: Bad packet id: {}", packet.id);
				assert(false);
		}
		if (i++ > 100)
			break;
	}

	Console.WriteLnFmt("GSDumpFileParsed(): Parsed {} packets ", m_packets.size());
}

void GSDumpFileParsed::AddToModel(QStandardItemModel* model)
{
	for (const std::unique_ptr<GSDumpFileParsed::GSDumpPacket>& packet : m_packets)
	{
		packet->AddToModel(model);
	}
}

// TODO: Make an enum with all possible register fields
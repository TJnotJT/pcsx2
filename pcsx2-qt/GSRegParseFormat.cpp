#include <sstream>
#include "GSRegParseFormat.h"

// std::map<u32, std::string> gsRegInfoPrimNames = {
// 	{ GS_POINTLIST     , "POINT" },
// 	{ GS_LINELIST      , "LINE" },
// 	{ GS_LINESTRIP     , "LINESTRIP" },
// 	{ GS_TRIANGLELIST  , "TRIANGLE" },
// 	{ GS_TRIANGLESTRIP , "TRIANGLESTRIP" },
// 	{ GS_TRIANGLEFAN   , "TRIANGLEFAN" },
// 	{ GS_SPRITE        , "SPRITE" },
// 	{ GS_INVALID       , "INVALID" },
// };

// std::map<u32, std::string> gsRegInfoFlgNames = {
// 	{ GIF_FLG_PACKED,  "PACKED" },
// 	{ GIF_FLG_REGLIST, "REGLIST" },
// 	{ GIF_FLG_IMAGE,   "IMAGE" },
// 	{ GIF_FLG_REGID,   "REGID" },
// }; 

using CompressedRegIdNameList = std::vector<std::tuple<u32, std::string>>;
using CompressedFieldList = std::vector<std::tuple<std::string, u32, u32, GSRegField::Format>>;

std::vector<std::tuple<CompressedRegIdNameList, CompressedFieldList>> gsRegInfoCompressed =
{
	{{{GIF_A_D_REG_ALPHA_1, "ALPHA_1"}, {GIF_A_D_REG_ALPHA_2, "ALPHA_2"}}, {
		{"A", 0, 2, GSRegField::Format::AlphaABD},
		{"B", 2, 4, GSRegField::Format::AlphaABD},
		{"C", 4, 6, GSRegField::Format::AlphaC},
		{"D", 6, 8, GSRegField::Format::AlphaABD},
		{"FIX", 32, 40, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_BITBLTBUF, "BITBLTBUF"}}, {
		{"SBP", 0, 14, GSRegField::Format::Address},
		{"SBW", 16, 22, GSRegField::Format::BufferWidth},
		{"SPSM", 24, 30, GSRegField::Format::PSMC},
		{"DBP", 32, 46, GSRegField::Format::Address},
		{"DBW", 48, 54, GSRegField::Format::BufferWidth},
		{"DPSM", 56, 62, GSRegField::Format::PSMC},
	}},
	{{{GIF_A_D_REG_CLAMP_1, "CLAMP_1"}, {GIF_A_D_REG_CLAMP_2, "CLAMP_2"}}, {
		{"WMS", 0, 2, GSRegField::Format::WrapMode},
		{"WMT", 2, 3, GSRegField::Format::WrapMode},
		{"MINU", 4, 14, GSRegField::Format::Unsigned},
		{"MAXU", 14, 24, GSRegField::Format::Unsigned},
		{"MINV", 24, 34, GSRegField::Format::Unsigned},
		{"MAXV", 34, 44, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_CLAMP_1, "COLCLAMP"}}, {
		{"CLAMP", 0, 1, GSRegField::Format::CLAMP},
	}},
	{{{GIF_A_D_REG_DIMX, "DIMX"}}, {
		{"DM00", 0, 3, GSRegField::Format::Signed_2_1},
		{"DM01", 4, 7, GSRegField::Format::Signed_2_1},
		{"DM02", 8, 11, GSRegField::Format::Signed_2_1},
		{"DM03", 12, 15, GSRegField::Format::Signed_2_1},
		{"DM10", 16, 19, GSRegField::Format::Signed_2_1},
		{"DM11", 20, 23, GSRegField::Format::Signed_2_1},
		{"DM12", 24, 27, GSRegField::Format::Signed_2_1},
		{"DM13", 28, 31, GSRegField::Format::Signed_2_1},
		{"DM20", 32, 35, GSRegField::Format::Signed_2_1},
		{"DM21", 36, 39, GSRegField::Format::Signed_2_1},
		{"DM22", 40, 43, GSRegField::Format::Signed_2_1},
		{"DM23", 44, 47, GSRegField::Format::Signed_2_1},
		{"DM30", 48, 51, GSRegField::Format::Signed_2_1},
		{"DM31", 52, 55, GSRegField::Format::Signed_2_1},
		{"DM32", 56, 59, GSRegField::Format::Signed_2_1},
		{"DM33", 60, 63, GSRegField::Format::Signed_2_1},
	}},
	{{{GIF_A_D_REG_DTHE, "DTHE"}}, {
		{"DTHE", 0, 1, GSRegField::Format::DTHE},
	}},
	{{{GIF_A_D_REG_FBA_1, "FBA_1"}, {GIF_A_D_REG_FBA_2, "FBA_2"}}, {
		{"FBA", 0, 1, GSRegField::Format::FBA},
	}},
	{{{GIF_A_D_REG_FINISH, "FINISH"}}, {
		{"PAD", 0, 1, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_FOG, "FOG"}}, {
		{"F", 0, 8, GSRegField::Format::Color},
	}},
	{{{GIF_A_D_REG_FOGCOL, "FOGCOL"}}, {
		{"FCR", 0, 8, GSRegField::Format::Color},
		{"FCG", 8, 16, GSRegField::Format::Color},
		{"FCB", 16, 24, GSRegField::Format::Color},
	}},
	{{{GIF_A_D_REG_FRAME_1, "FRAME_1"}, {GIF_A_D_REG_FRAME_2, "FRAME_2"}}, {
		{"FBP", 0, 9, GSRegField::Format::Address},
		{"FBW", 16, 22, GSRegField::Format::BufferWidth},
		{"PSM", 24, 30, GSRegField::Format::PSMC},
		{"FBMSK", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_HWREG, "HWREG"}}, {
		{"DATA", 0, 64, GSRegField::Format::Unsigned}, // No fields defined, just a placeholder
	}},
	{{{GIF_A_D_REG_LABEL, "LABEL"}}, {
		{"ID", 0, 32, GSRegField::Format::Unsigned},
		{"IDMSK", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_MIPTBP1_1, "MIPTBP1_1"}, {GIF_A_D_REG_MIPTBP1_2, "MIPTBP1_2"}}, {
		{"TBP1", 0, 14, GSRegField::Format::Address},
		{"TBW1", 14, 20, GSRegField::Format::BufferWidth},
		{"TBP2", 20, 34, GSRegField::Format::Address},
		{"TBW2", 34, 40, GSRegField::Format::BufferWidth},
		{"TBP3", 40, 54, GSRegField::Format::Address},
		{"TBW3", 54, 60, GSRegField::Format::BufferWidth},
	}},
	{{{GIF_A_D_REG_MIPTBP2_1, "MIPTBP2_1"}, {GIF_A_D_REG_MIPTBP2_2, "MIPTBP2_2"}}, {
		{"TBP4", 0, 14, GSRegField::Format::Address},
		{"TBW4", 14, 20, GSRegField::Format::BufferWidth},
		{"TBP5", 20, 34, GSRegField::Format::Address},
		{"TBW5", 34, 40, GSRegField::Format::BufferWidth},
		{"TBP6", 40, 54, GSRegField::Format::Address},
		{"TBW6", 54, 60, GSRegField::Format::BufferWidth},
	}},
	{{{GIF_A_D_REG_PABE, "PABE"}}, {
		{"PABE", 0, 1, GSRegField::Format::PABE},
	}},
	{{{GIF_A_D_REG_PRIM, "PRIM"}}, {
		{"PRIM", 0, 3, GSRegField::Format::PRIM},
		{"IIP", 3, 4, GSRegField::Format::IIP},
		{"TME", 4, 5, GSRegField::Format::TME},
		{"FGE", 5, 6, GSRegField::Format::FGE},
		{"ABE", 6, 7, GSRegField::Format::ABE},
		{"AA1", 7, 8, GSRegField::Format::AA1},
		{"FST", 8, 9, GSRegField::Format::FST},
		{"CTXT", 9, 10, GSRegField::Format::Unsigned},
		{"FIX", 10, 11, GSRegField::Format::FIX},
	}},
	{{{GIF_A_D_REG_RGBAQ, "RGBAQ"}}, {
		{"R", 0, 8, GSRegField::Format::Color},
		{"G", 8, 16, GSRegField::Format::Color},
		{"B", 16, 24, GSRegField::Format::Color},
		{"A", 24, 32, GSRegField::Format::Unsigned},
		{"Q", 32, 64, GSRegField::Format::Float},
	}},
	{{{GIF_A_D_REG_ST, "ST"}}, {
		{"S", 0, 32, GSRegField::Format::Float},
		{"T", 32, 64, GSRegField::Format::Float},
	}},
	{{{GIF_A_D_REG_TEX0_1, "TEX0_1"}, {GIF_A_D_REG_TEX0_2, "TEX0_2"}}, {
		{"TBP0", 0, 14, GSRegField::Format::Address},
		{"TBPW", 14, 20, GSRegField::Format::BufferWidth},
		{"PSM", 20, 26, GSRegField::Format::PSMC},
		{"TW", 26, 30, GSRegField::Format::Log2Size},
		{"TH", 30, 34, GSRegField::Format::Log2Size},
		{"TCC", 34, 36, GSRegField::Format::TCC},
		{"TFX", 35, 37, GSRegField::Format::TFX},
		{"CBP", 37, 51, GSRegField::Format::Address},
		{"CPSM", 51, 55, GSRegField::Format::PSMC},
		{"CSM", 55, 56, GSRegField::Format::CSM},
		{"CSA", 56, 61, GSRegField::Format::TexelOffset},
		{"CLD", 61, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEX1_1, "TEX1_1"}, {GIF_A_D_REG_TEX1_2, "TEX1_2"}}, {
		{"LCM", 0, 2, GSRegField::Format::LCM},
		{"MXL", 2, 5, GSRegField::Format::Unsigned},
		{"MMAG", 5, 6, GSRegField::Format::MMAG},
		{"MMIN", 6, 9, GSRegField::Format::MMIN},
		{"MTBA", 9, 10, GSRegField::Format::MTBA},
		{"L", 19, 21, GSRegField::Format::Unsigned},
		{"K", 32, 44, GSRegField::Format::Signed_7_4},
	}},
	{{{GIF_A_D_REG_TEX2_1, "TEX2_1"}, {GIF_A_D_REG_TEX2_2, "TEX2_2"}}, {
		{"PSM", 20, 26, GSRegField::Format::PSMC},
		{"CBP", 37, 51, GSRegField::Format::Address},
		{"CPSM", 51, 55, GSRegField::Format::PSMC},
		{"CSM", 55, 56, GSRegField::Format::CSM},
		{"CSA", 56, 60, GSRegField::Format::TexelOffset},
		{"CLD", 61, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXA, "TEXA"}}, {
		{"TA0", 0, 8, GSRegField::Format::Unsigned},
		{"AEM", 15, 16, GSRegField::Format::AEM},
		{"TA1", 32, 40, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXCLUT, "TEXCLUT"}}, {
		{"CBW", 0, 8, GSRegField::Format::BufferWidth},
		{"COU", 8, 24, GSRegField::Format::TexelOffset},
		{"COV", 12, 22, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXFLUSH, "TEXFLUSH"}}, {
		{"PAD", 0, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TRXDIR, "TRXDIR"}}, {
		{"XDIR", 0, 2, GSRegField::Format::XDIR},
	}},
	{{{GIF_A_D_REG_TRXPOS, "TRXPOS"}}, {
		{"SSAX", 0, 11, GSRegField::Format::Unsigned},
		{"SSAY", 16, 27, GSRegField::Format::Unsigned},
		{"DSAX", 32, 43, GSRegField::Format::Unsigned},
		{"DSAY", 48, 59, GSRegField::Format::Unsigned},
		{"DIR", 59, 61, GSRegField::Format::DIR},
	}},
	{{{GIF_A_D_REG_TRXREG, "TRXREG"}}, {
		{"RRW", 0, 11, GSRegField::Format::Unsigned},
		{"RRH", 32, 44, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_UV, "UV"}}, {
		{"U", 0, 14, GSRegField::Format::Unsigned_4},
		{"V", 16, 30, GSRegField::Format::Unsigned_4},
	}},
	{{{GIF_A_D_REG_XYOFFSET_1, "XYOFFSET_1"}, {GIF_A_D_REG_XYOFFSET_2, "XYOFFSET_2"}}, {
		{"OFX", 0, 16, GSRegField::Format::Unsigned_4},
		{"OFY", 32, 48, GSRegField::Format::Unsigned_4},
	}},
	{{{GIF_A_D_REG_XYZF2, "XYZF2"}, {GIF_A_D_REG_XYZF3, "XYZF3"}}, {
		{"X", 0, 16, GSRegField::Format::Unsigned_4},
		{"Y", 16, 32, GSRegField::Format::Unsigned_4},
		{"Z", 32, 56, GSRegField::Format::Unsigned},
		{"F", 56, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_XYZ2, "XYZ2"}, {GIF_A_D_REG_XYZ3, "XYZ3"}}, {
		{"X", 0, 16, GSRegField::Format::Unsigned_4},
		{"Y", 16, 32, GSRegField::Format::Unsigned_4},
		{"Z", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_ZBUF_1, "ZBUF_1"}, {GIF_A_D_REG_ZBUF_2, "ZBUF_2"}}, {
		{"ZBP", 0, 10, GSRegField::Format::Address},
		{"PSM", 24, 28, GSRegField::Format::PSMZ},
		{"ZMSK", 32, 33, GSRegField::Format::ZMSK},
	}},
};

std::vector<std::tuple<u32,	std::string, CompressedFieldList>> gsPackedRegInfoCompressed =
{
	{GIF_REG_PRIM, "PRIM", {
		{"PRIM", 0, 3, GSRegField::Format::PRIM},
		{"IIP", 3, 4, GSRegField::Format::IIP},
		{"TME", 4, 5, GSRegField::Format::TME},
		{"FGE", 5, 6, GSRegField::Format::FGE},
		{"ABE", 6, 7, GSRegField::Format::ABE},
		{"AA1", 7, 8, GSRegField::Format::AA1},
		{"FST", 8, 9, GSRegField::Format::FST},
		{"CTXT", 9, 10, GSRegField::Format::Unsigned},
		{"FIX", 10, 11, GSRegField::Format::FIX},
	}},
	{GIF_REG_RGBA, "RGBA", {
		{"R", 0, 8, GSRegField::Format::Color},
		{"G", 32, 40, GSRegField::Format::Color},
		{"B", 64, 72, GSRegField::Format::Color},
		{"A", 96, 104, GSRegField::Format::Color},
	}},
	{GIF_REG_STQ, "STQ", {
		{"S", 0, 32, GSRegField::Format::Float},
		{"T", 32, 64, GSRegField::Format::Float},
		{"Q", 64, 96, GSRegField::Format::Float},
	}},
	{GIF_REG_UV, "UV", {
		{"U", 0, 14, GSRegField::Format::Unsigned_4},
		{"V", 32, 46, GSRegField::Format::Unsigned_4},
	}},
	{GIF_REG_XYZF2, "XYZF2", {
		{"X", 0, 16, GSRegField::Format::Unsigned_4},
		{"Y", 32, 48, GSRegField::Format::Unsigned_4},
		{"Z", 68, 92, GSRegField::Format::Unsigned},
		{"F", 100, 108, GSRegField::Format::Color},
		{"ADC", 111, 112, GSRegField::Format::ADC},
	}},
	{GIF_REG_XYZ2, "XYZ2", {
		{"X", 0, 16, GSRegField::Format::Unsigned_4},
		{"Y", 32, 48, GSRegField::Format::Unsigned_4},
		{"Z", 64, 96, GSRegField::Format::Unsigned},
		{"ADC", 111, 112, GSRegField::Format::ADC},
	}},
	{GIF_REG_FOG, "FOG", {
		{"F", 100, 108, GSRegField::Format::Color},
	}},
	{GIF_REG_A_D, "A_D", {
		{"DATA", 0, 64, GSRegField::Format::A_D},
		{"ADDR", 64, 96, GSRegField::Format::PackedRegID},
	}},
	{GIF_REG_NOP, "NOP", {
		{"PAD", 0, 64, GSRegField::Format::Unsigned},
	}},
};

std::map<u32, GSRegInfo> gsPackedRegInfo;
std::map<u32, GSRegInfo> gsRegInfo;
std::map<std::string, u32> gsRegNameToId;

static void GSRegInfoInit()
{
	static bool init = false;
	if (init)
		return;
	init = true;

	auto decompressFields = [](const CompressedFieldList& src,
								std::map<std::string, GSRegField>* dst) {
		for (const auto& [name, start_bit, end_bit, format] : src)
		{
			dst->insert({name, {name, start_bit, end_bit, format}});
		}
	};

	// initialize gsRegInfo and gsRegNameToId
	for (const auto& [regIdNamesCompr, fieldsCompr] : gsRegInfoCompressed)
	{
		std::map<std::string, GSRegField> fields;
		decompressFields(fieldsCompr, &fields);

		for (const auto& [id, name]: regIdNamesCompr)
		{
			gsRegInfo.insert({id, {id, name, fields}});
			gsRegNameToId.insert({name, id});
		}
	}

	// Initialize gsPackedRegInfo
	for (const auto& [id, name, fieldsCompr] : gsPackedRegInfoCompressed)
	{
		std::map<std::string, GSRegField> fields;
		decompressFields(fieldsCompr, &fields);
		gsPackedRegInfo.insert({id, {id, name, fields}});
	}
}

bool GSRegDecodeFields(u32 id, u64* data, std::map<std::string, u64>* field_vals)
{
	GSRegInfoInit();
	field_vals->clear();
	const GSRegInfo& info = gsRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
		u64 bits = (data[0] >> field.start_bit) & mask;
		field_vals->insert({field.name, bits});
	}
	return true;
}

bool GSPackedRegDecodeFields(uint id, u64* data, std::map<std::string, u64>* field_vals)
{
	GSRegInfoInit();
	field_vals->clear();
	const GSRegInfo& info = gsPackedRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
		u64 bits = 0;
		if (field.start_bit < 64)
		{
			if (field.end_bit > 64)
			{
				// This is an error, none of the fields should cross the 64-bit boundary
				Console.Error("GSRegDecodeFields128: Field %s crosses 64-bit boundary", field.name.c_str());
				return false;
			}
			bits = (data[0] >> field.start_bit) & mask;
		}
		else
		{
			bits = (data[1] >> (field.start_bit - 64)) & mask;
		}
		field_vals->insert({field.name, bits});
	}
	return true;
}

bool GSRegEncodeFields(u32 id, const std::map<std::string, u64>& field_vals, u64* data)
{
	GSRegInfoInit();
	*data = 0;
	const GSRegInfo& info = gsRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		auto it = field_vals.find(field_name);
		if (it != field_vals.end())
		{
			const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
			*data |= (it->second & mask) << field.start_bit;
		}
	}
	return true;
}

bool GSPackedRegEndcodeFields(u32 id, const std::map<std::string, u64>& field_vals, u64* data)
{
	GSRegInfoInit();
	data[0] = 0;
	data[1] = 0;
	const GSRegInfo& info = gsPackedRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		auto it = field_vals.find(field_name);
		if (it != field_vals.end())
		{
			const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
			u64 bits = it->second & mask;
			if (field.start_bit < 64)
			{
				if (field.end_bit > 64)
				{
					// This is an error, none of the fields should cross the 64-bit boundary
					Console.Error("GSPackedRegEncodeFields: Field %s crosses 64-bit boundary", field.name.c_str());
					return false;
				}
				data[0] |= bits << field.start_bit;
			}
			else
			{
				data[1] |= bits << (field.start_bit - 64);
			}
		}
	}
	return true;
}

std::string GSRegGetName(u32 id)
{
	GSRegInfoInit();
	return gsRegInfo[id].name;
}

std::string GSRegDoFormat(u32 id, u64 data)
{
	GSRegInfoInit();
	std::vector<std::tuple<std::string, u64>> values;
	std::stringstream ss;
	bool first = true;
	for (const std::tuple<std::string, u64>& val : values)
	{
		const std::string name = std::get<0>(val);
		const u64 bits = std::get<1>(val);
		if (!first)
			ss << " ";
		ss << name << "=" << bits;
	}
	return ss.str();
}

std::string GSRegFieldDoFormat(u32 id, u64 bits, GSRegField::Format fmt)
{
	GSRegInfoInit();
	switch (fmt)
	{
		// TODO
		default:
			return "";
	}
}
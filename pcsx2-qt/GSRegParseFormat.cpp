#include <sstream>
#include "GSRegParseFormat.h"

// TODO: WE can streamline this a lot more. Not need to duplicate the keys in the values.
// We can make the duplication in the uncompressed version.
GSRegInfoCompressed gsRegInfoCompressed[] = {
	{{{GIF_A_D_REG_ALPHA_1, "ALPHA_1"}, {GIF_A_D_REG_ALPHA_2, "ALPHA_2"}}, {
		{"A", {"A", 0, 2, GSRegField::FormatAlphaABD}},
		{"B", {"B", 2, 4, GSRegField::FormatAlphaABD}},
		{"C", {"C", 4, 6, GSRegField::FormatAlphaC}},
		{"D", {"D", 6, 8, GSRegField::FormatAlphaABD}},
		{"FIX", {"FIX", 32, 40, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_BITBLTBUF, "BITBLTBUF"}}, {
		{"SBP", {"SBP", 0, 14, GSRegField::FormatAddress}},
		{"SBW", {"SBW", 16, 22, GSRegField::FormatBufferWidth}},
		{"SPSM", {"SPSM", 24, 30, GSRegField::FormatPSMC}},
		{"DBP", {"DBP", 32, 46, GSRegField::FormatAddress}},
		{"DBW", {"DBW", 48, 54, GSRegField::FormatBufferWidth}},
		{"DPSM", {"DPSM", 56, 62, GSRegField::FormatPSMC}},
	}},
	{{{GIF_A_D_REG_CLAMP_1, "CLAMP_1"}, {GIF_A_D_REG_CLAMP_2, "CLAMP_2"}}, {
		{"WMS", {"WMS", 0, 2, GSRegField::FormatWrapMode}},
		{"WMT", {"WMT", 2, 3, GSRegField::FormatWrapMode}},
		{"MINU", {"MINU", 4, 14, GSRegField::FormatUnsigned}},
		{"MAXU", {"MAXU", 14, 24, GSRegField::FormatUnsigned}},
		{"MINV", {"MINV", 24, 34, GSRegField::FormatUnsigned}},
		{"MAXV", {"MAXV", 34, 44, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_CLAMP_1, "COLCLAMP"}}, {
		{"CLAMP", {"CLAMP", 0, 1, GSRegField::FormatCLAMP}},
	}},
	{{{GIF_A_D_REG_DIMX, "DIMX"}}, {
		{"DM00", {"DM00", 0, 3, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM01", {"DM01", 4, 7, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM02", {"DM02", 8, 11, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM03", {"DM03", 12, 15, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM10", {"DM10", 16, 19, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM11", {"DM11", 20, 23, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM12", {"DM12", 24, 27, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM13", {"DM13", 28, 31, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM20", {"DM20", 32, 35, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM21", {"DM21", 36, 39, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM22", {"DM22", 40, 43, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM23", {"DM23", 44, 47, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM30", {"DM30", 48, 51, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM31", {"DM31", 52, 55, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM32", {"DM32", 56, 59, GSRegField::FormatSigned_2_1}}, // 1:7:4
		{"DM33", {"DM33", 60, 63, GSRegField::FormatSigned_2_1}}, // 1:7:4
	}},
	{{{GIF_A_D_REG_DTHE, "DTHE"}}, {
		{"DTHE", {"DTHE", 0, 1, GSRegField::FormatDTHE}},
	}},
	{{{GIF_A_D_REG_FBA_1, "FBA_1"}, {GIF_A_D_REG_FBA_2, "FBA_2"}}, {
		{"FBA", {"FBA", 0, 1, GSRegField::FormatFBA}},
	}},
	{{{GIF_A_D_REG_FINISH, "FINISH"}}, {
		{"PAD", {"PAD", 0, 1, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_FOG, "FOG"}}, {
		{"F", {"F", 0, 8, GSRegField::FormatColor}},
	}},
	{{{GIF_A_D_REG_FOGCOL, "FOGCOL"}}, {
		{"FCR", {"FCR", 0, 8, GSRegField::FormatColor}},
		{"FCG", {"FCG", 8, 16, GSRegField::FormatColor}},
		{"FCB", {"FCB", 16, 24, GSRegField::FormatColor}},
	}},
	{{{GIF_A_D_REG_FRAME_1, "FRAME_1"}, {GIF_A_D_REG_FRAME_2, "FRAME_2"}}, {
		{"FBP", {"FBP", 0, 9, GSRegField::FormatAddress}},
		{"FBW", {"FBW", 16, 22, GSRegField::FormatBufferWidth}},
		{"PSM", {"PSM", 24, 30, GSRegField::FormatPSMC}},
		{"FBMSK", {"FBMSK", 32, 64, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_HWREG, "HWREG"}}, {
		{"DATA", {"DATA", 0, 64, GSRegField::FormatUnsigned}}, // No fields defined, just a placeholder
	}},
	{{{GIF_A_D_REG_LABEL, "LABEL"}}, {
		{"ID", {"ID", 0, 32, GSRegField::FormatUnsigned}},
		{"IDMSK", {"IDMSK", 32, 64, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_MIPTBP1_1, "MIPTBP1_1"}, {GIF_A_D_REG_MIPTBP1_2, "MIPTBP1_2"}}, {
		{"TBP1", {"TBP1", 0, 14, GSRegField::FormatAddress}},
		{"TBW1", {"TBW1", 14, 20, GSRegField::FormatBufferWidth}},
		{"TBP2", {"TBP2", 20, 34, GSRegField::FormatAddress}},
		{"TBW2", {"TBW2", 34, 40, GSRegField::FormatBufferWidth}},
		{"TBP3", {"TBP3", 40, 54, GSRegField::FormatAddress}},
		{"TBW3", {"TBW3", 54, 60, GSRegField::FormatBufferWidth}},
	}},
	{{{GIF_A_D_REG_MIPTBP2_1, "MIPTBP2_1"}, {GIF_A_D_REG_MIPTBP2_2, "MIPTBP2_2"}}, {
		{"TBP4", {"TBP4", 0, 14, GSRegField::FormatAddress}},
		{"TBW4", {"TBW4", 14, 20, GSRegField::FormatBufferWidth}},
		{"TBP5", {"TBP5", 20, 34, GSRegField::FormatAddress}},
		{"TBW5", {"TBW5", 34, 40, GSRegField::FormatBufferWidth}},
		{"TBP6", {"TBP6", 40, 54, GSRegField::FormatAddress}},
		{"TBW6", {"TBW6", 54, 60, GSRegField::FormatBufferWidth}},
	}},
	{{{GIF_A_D_REG_PABE, "PABE"}}, {
		{"PABE", {"PABE", 0, 1, GSRegField::FormatPABE}},
	}},
	{{{GIF_A_D_REG_PRIM, "PRIM"}}, {
		{"PRIM", {"PRIM", 0, 3, GSRegField::FormatPRIM}},
		{"IIP", {"IIP", 3, 4, GSRegField::FormatIIP}},
		{"TME", {"TME", 4, 5, GSRegField::FormatTME}},
		{"FGE", {"FGE", 5, 6, GSRegField::FormatFGE}},
		{"ABE", {"ABE", 6, 7, GSRegField::FormatABE}},
		{"AA1", {"AA1", 7, 8, GSRegField::FormatAA1}},
		{"FST", {"FST", 8, 9, GSRegField::FormatFST}},
		{"CTXT", {"CTXT", 9, 10, GSRegField::FormatUnsigned}},
		{"FIX", {"FIX", 10, 11, GSRegField::FormatFIX}},
	}},
	{{{GIF_A_D_REG_RGBAQ, "RGBAQ"}}, {
		{"R", {"R", 0, 8, GSRegField::FormatColor}},
		{"G", {"G", 8, 16, GSRegField::FormatColor}},
		{"B", {"B", 16, 24, GSRegField::FormatColor}},
		{"A", {"A", 24, 32, GSRegField::FormatUnsigned}},
		{"Q", {"Q", 32, 64, GSRegField::FormatFloat}},
	}},
	{{GIF_A_D_REG_ST, "ST"}, {
		{"S", {"S", 0, 32, GSRegField::FormatFloat}},
		{"T", {"T", 32, 64, GSRegField::FormatFloat}},
	}},
	{{{GIF_A_D_REG_TEX0_1, "TEX0_1"}, {GIF_A_D_REG_TEX0_2, "TEX0_2"}}, {
		{"TBP0", {"TBP0", 0, 14, GSRegField::FormatAddress}},
		{"TBPW", {"TBPW", 14, 20, GSRegField::FormatBufferWidth}},
		{"PSM", {"PSM", 20, 26, GSRegField::FormatPSMC}},
		{"TW", {"TW", 26, 30, GSRegField::FormatLog2Size}},
		{"TH", {"TH", 30, 34, GSRegField::FormatLog2Size}},
		{"TCC", {"TCC", 34, 36, GSRegField::FormatTCC}},
		{"TFX", {"TFX", 35, 37, GSRegField::FormatTFX}},
		{"CBP", {"CBP", 37, 51, GSRegField::FormatAddress}},
		{"CPSM", {"CPSM", 51, 55, GSRegField::FormatPSMC}},
		{"CSM", {"CSM", 55, 56, GSRegField::FormatCSM}},
		{"CSA", {"CSA", 56, 61, GSRegField::FormatTexelOffset}},
		{"CLD", {"CLD", 61, 64, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_TEX1_1, "TEX1_1"}, {GIF_A_D_REG_TEX1_2, "TEX1_2"}}, {
		{"LCM", {"LCM", 0, 2, GSRegField::FormatLCM}},
		{"MXL", {"MXL", 2, 5, GSRegField::FormatUnsigned}},
		{"MMAG", {"MMAG", 5, 6, GSRegField::FormatMMAG}},
		{"MMIN", {"MMIN", 6, 9, GSRegField::FormatMMIN}},
		{"MTBA", {"MTBA", 9, 10, GSRegField::FormatMTBA}},
		{"L", {"L", 19, 21, GSRegField::FormatUnsigned}},
		{"K", {"K", 32, 44, GSRegField::FormatSigned_7_4}}, // 1:7:4
	}},
	{{{GIF_A_D_REG_TEX2_1, "TEX2_1"}, {GIF_A_D_REG_TEX2_2, "TEX2_2"}}, {
		{"PSM", {"PSM", 20, 26, GSRegField::FormatPSMC}},
		{"CBP", {"CBP", 37, 51, GSRegField::FormatAddress}},
		{"CPSM", {"CPSM", 51, 55, GSRegField::FormatPSMC}},
		{"CSM", {"CSM", 55, 56, GSRegField::FormatCSM}},
		{"CSA", {"CSA", 56, 60, GSRegField::FormatTexelOffset}},
		{"CLD", {"CLD", 61, 64, GSRegField::FormatUnsigned}},
	}};
	{{{GS_A_D_TEXA, "TEXA"}}, {
		{"TA0", {"TA0", 0, 8, GSRegField::FormatUnsigned}},
		{"AEM", {"AEM", 15, 16, GSRegField::FormatAEM}},
		{"TA1", {"TA1", 32, 40, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_TEXCLUT, "TEXCLUT"}}, {
		{"CBW", {"CBW", 0, 8, GSRegField::FormatBufferWidth}},
		{"COU", {"COU", 8, 24, GSRegField::FormatTexelOffset}},
		{"COV", {"COV", 12, 22, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_TEXFLUSH, "TEXFLUSH"}}, {
		{"PAD", {"PAD", 0, 64, GSRegField::FormatUnsigned}}, // No fields defined, just a placeholder
	}},
	{{{GIF_A_D_REG_TRXDIR, "TRXDIR"}}, {
		{"XDIR", {"XDIR", 0, 2, GSRegField::FormatXDIR}},
	}},
	{{{GIF_A_D_REG_TRXPOS, "TRXPOS"}}, {
		{"SSAX", {"SSAX", 0, 11, GSRegField::FormatUnsigned}},
		{"SSAY", {"SSAY", 16, 27, GSRegField::FormatUnsigned}},
		{"DSAX", {"DSAX", 32, 43, GSRegField::FormatUnsigned}},
		{"DSAY", {"DSAY", 48, 59, GSRegField::FormatUnsigned}},
		{"DIR", {"DIR", 59, 61, GSRegField::FormatDIR}},
	}},
	{{{GIF_A_D_REG_TRXREG, "TRXREG"}}, {
		{"RRW", {"RRW", 0, 11, GSRegField::FormatUnsigned}},
		{"RRH", {"RRH", 32, 44, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_UV, "UV"}}, {
		{"U", {"U", 0, 14, GSRegField::FormatUnsigned_4}},
		{"V", {"V", 16, 30, GSRegField::FormatUnsigned_4}},
	}},
	{{{GIF_A_D_REG_XYOFFSET_1, "XYOFFSET_1"}, {GIF_A_D_REG_XYOFFSET_2, "XYOFFSET_2"}}, {
		{"OFX", {"OFX", 0, 16, GSRegField::FormatUnsigned_4}},
		{"OFY", {"OFY", 32, 48, GSRegField::FormatUnsigned_4}},
	}},
	{{{GIF_A_D_REG_XYZF2, "XYZF2"}, {GIF_A_D_REG_XYZF3, "XYZF3"}}, {
		{"X", {"X", 0, 16, GSRegField::FormatUnsigned_4}},
		{"Y", {"Y", 16, 32, GSRegField::FormatUnsigned_4}},
		{"Z", {"Z", 32, 56, GSRegField::FormatUnsigned}},
		{"F", {"F", 56, 64, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_XYZ2, "XYZ2"}, {GIF_A_D_REG_XYZ3, "XYZ3"}}, {
		{"X", {"X", 0, 16, GSRegField::FormatUnsigned_4}},
		{"Y", {"Y", 16, 32, GSRegField::FormatUnsigned_4}},
		{"Z", {"Z", 32, 64, GSRegField::FormatUnsigned}},
	}},
	{{{GIF_A_D_REG_ZBUF_1, "ZBUF_1"}, {GIF_A_D_REG_ZBUF_2, "ZBUF_2"}}, {
		{"ZBP", {"ZBP", 0, 10, GSRegField::FormatAddress}},
		{"PSM", {"PSM", 24, 28, GSRegField::FormatPSMZ}},
		{"ZMSK", {"ZMSK", 32, 33, GSRegField::FormatZMSK}},
	}},
};

std::map<u32, GSRegInfo> gsPackedRegInfo = {
	{GIF_REG_PRIM, {GIF_REG_PRIM, "PRIM", {
		{"PRIM", {"PRIM", 0, 3, GSRegField::FormatPRIM}},
		{"IIP", {"IIP", 3, 4, GSRegField::FormatIIP}},
		{"TME", {"TME", 4, 5, GSRegField::FormatTME}},
		{"FGE", {"FGE", 5, 6, GSRegField::FormatFGE}},
		{"ABE", {"ABE", 6, 7, GSRegField::FormatABE}},
		{"AA1", {"AA1", 7, 8, GSRegField::FormatAA1}},
		{"FST", {"FST", 8, 9, GSRegField::FormatFST}},
		{"CTXT", {"CTXT", 9, 10, GSRegField::FormatUnsigned}},
		{"FIX", {"FIX", 10, 11, GSRegField::FormatFIX}},
	}}},
	{GIF_REG_RGBAQ, {GIF_REG_RGBAQ, "RGBAQ", {
		{"R", {"R", 0, 8, GSRegField::FormatColor}},
		{"G", {"G", 32, 40, GSRegField::FormatColor}},
		{"B", {"B", 64, 72, GSRegField::FormatColor}},
		{"A", {"A", 96, 104, GSRegField::FormatColor}},
	}}},
	{GIF_REG_ST, {GIF_REG_ST, "ST", {
		{"S", {"S", 0, 32, GSRegField::FormatFloat}},
		{"T", {"T", 32, 64, GSRegField::FormatFloat}},
		{"Q", {"Q", 64, 96, GSRegField::FormatFloat}},
	}}},
	{GIF_REG_UV, {GIF_REG_UV, "UV", {
		{"U", {"U", 0, 14, GSRegField::FormatUnsigned_4}},
		{"V", {"V", 32, 46, GSRegField::FormatUnsigned_4}},
	}}},
	{GIF_REG_XYZF2, {GIF_REG_XYZF2, "XYZF2", {
		{"X", {"X", 0, 16, GSRegField::FormatUnsigned_4}},
		{"Y", {"Y", 32, 48, GSRegField::FormatUnsigned_4}},
		{"Z", {"Z", 68, 92, GSRegField::FormatUnsigned}},
		{"F", {"F", 100, 108, GSRegField::FormatColor}},
		{"ADC", {"ADC", 111, 112, GSRegField::FormatADC}},
	}}},
	{GIF_REG_XYZ2, {GIF_REG_XYZ2, "XYZ2", {
		{"X", {"X", 0, 16, GSRegField::FormatUnsigned_4}},
		{"Y", {"Y", 32, 48, GSRegField::FormatUnsigned_4}},
		{"Z", {"Z", 64, 96, GSRegField::FormatUnsigned}},
		{"ADC", {"ADC", 111, 112, GSRegField::FormatADC}},
	}}},
	{GIF_REG_FOG, {GIF_REG_FOG, "FOG", {
		{"F", {"F", 100, 108, GSRegField::FormatColor}},
	}}},
	{GIF_REG_A_D, {GIF_REG_A_D, "A_D", {
		{"DATA", {"DATA", 0, 64, GSRegField::FormatA_D}},
		{"ADDR", {"ADDR", 64, 96, GSRegField::FormatPackedRegID}},
	}}},
	{GIF_REG_NOP, {GIF_REG_NOP, "NOP", {
		{"PAD", {"PAD", 0, 64, GSRegField::FormatUnsigned}}, // No fields defined, just a placeholder
	}}},
};

std::map<u32, GSRegInfo> gsRegInfo;
std::map<std::string, u32> gsRegNameToID;

static void GSRegInfoInit()
{
	static bool init = false;
	if (init)
		return;
	init = true;
	for (int i = 0; i < sizeof(gsRegInfoCompressed) / sizeof(gsRegInfoCompressed[0]); i++)
	{
		const GSRegInfoCompressed regInfoCompr = gsRegInfoCompressed[i];
		for (const std::tuple<u32, std::string>& id_name : regInfoCompr.id_name)
		{
			const u32 id = std::get<0>(id_name);
			const std::string& name = std::get<1>(id_name);
			const std::vector<GSRegField>& fields = regInfoCompr.fields;

			GSRegInfo regInfo(id, name, fields);

			gsRegInfo.insert({id, regInfo});
			gsRegNameToID.insert({name, id});
		}
	}
}

void GSRegDecodeFieldsImpl(const std::map<u32, GSRegInfo>& regInfo, u32 id, u64 data, std::map<std::string, u64>& field_vals)
{
	GSRegInfoInit();
	field_vals.clear();
	const GSRegInfo& info = regInfo.at(id);
	for (const GSRegField& field : info.fields)
	{
		const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
		u64 bits = (data >> field.start_bit) & mask;
		field_vals.insert({field.name, bits});
	}
}

u64 GSRegEncodeFieldsImpl(const std::map<u32, GSRegInfo>& regInfo, u32 id, const std::map<std::string, u64>& field_vals)
{
	GSRegInfoInit();
	u64 data = 0;
	const GSRegInfo& info = regInfo.at(id);
	for (const GSRegField& field : info.fields)
	{
		auto it = field_vals.find(field.name);
		if (it != field_vals.end())
		{
			const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
			data |= (it->second & mask) << field.start_bit;
		}
	}
	return data;
}

void GSRegDecodeFields(u32 id, u64 data, std::map<std::string, u64>& field_vals)
{
	GSRegDecodeFieldsImpl(gsRegInfo, id, data, field_vals);
}

u64 GSPackedRegDecodeFields(u32 id, u64 data, std::map<std::string, u64>& field_vals)
{
	GSRegDecodeFieldsImpl(gsPackedRegInfo, id, data, field_vals);
}

u64 GSRegEncodeFields(u32 id, const std::map<std::string, u64>& field_vals)
{
	return GSRegEncodeFieldsImpl(gsRegInfo, id, field_vals);	
}

u64 GSPackedRegEncodeFields(u32 id, const std::map<std::string, u64>& field_vals)
{
	return GSRegEncodeFieldsImpl(gsPackedRegInfo, id, field_vals);
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

	//std::map<std::string, u32> gsRegNameToID = {
//	{"PRIM", GIF_A_D_REG_PRIM},
//	{"RGBAQ", GIF_A_D_REG_RGBAQ},
//	{"ST", GIF_A_D_REG_ST},
//	{"UV", GIF_A_D_REG_UV},
//	{"XYZF2", GIF_A_D_REG_XYZF2},
//	{"XYZ2", GIF_A_D_REG_XYZ2},
//	{"TEX0_1", GIF_A_D_REG_TEX0_1},
//	{"TEX0_2", GIF_A_D_REG_TEX0_2},
//	{"CLAMP_1", GIF_A_D_REG_CLAMP_1},
//	{"CLAMP_2", GIF_A_D_REG_CLAMP_2},
//	{"FOG", GIF_A_D_REG_FOG},
//	{"XYZF3", GIF_A_D_REG_XYZF3},
//	{"XYZ3", GIF_A_D_REG_XYZ3},
//	{"NOP", GIF_A_D_REG_NOP},
//	{"TEX1_1", GIF_A_D_REG_TEX1_1},
//	{"TEX1_2", GIF_A_D_REG_TEX1_2},
//	{"TEX2_1", GIF_A_D_REG_TEX2_1},
//	{"TEX2_2", GIF_A_D_REG_TEX2_2},
//	{"XYOFFSET_1", GIF_A_D_REG_XYOFFSET_1},
//	{"XYOFFSET_2", GIF_A_D_REG_XYOFFSET_2},
//	{"PRMODECONT", GIF_A_D_REG_PRMODECONT},
//	{"PRMODE", GIF_A_D_REG_PRMODE},
//	{"TEXCLUT", GIF_A_D_REG_TEXCLUT},
//	{"SCANMSK", GIF_A_D_REG_SCANMSK},
//	{"MIPTBP1_1", GIF_A_D_REG_MIPTBP1_1},
//	{"MIPTBP1_2", GIF_A_D_REG_MIPTBP1_2},
//	{"MIPTBP2_1", GIF_A_D_REG_MIPTBP2_1},
//	{"MIPTBP2_2", GIF_A_D_REG_MIPTBP2_2},
//	{"TEXA", GIF_A_D_REG_TEXA},
//	{"FOGCOL", GIF_A_D_REG_FOGCOL},
//	{"TEXFLUSH", GIF_A_D_REG_TEXFLUSH},
//	{"SCISSOR_1", GIF_A_D_REG_SCISSOR_1},
//	{"SCISSOR_2", GIF_A_D_REG_SCISSOR_2},
//	{"ALPHA_1", GIF_A_D_REG_ALPHA_1},
//	{"ALPHA_2", GIF_A_D_REG_ALPHA_2},
//	{"DIMX", GIF_A_D_REG_DIMX},
//	{"DTHE", GIF_A_D_REG_DTHE},
//	{"COLCLAMP", GIF_A_D_REG_COLCLAMP},
//	{"TEST_1", GIF_A_D_REG_TEST_1},
//	{"TEST_2", GIF_A_D_REG_TEST_2},
//	{"PABE", GIF_A_D_REG_PABE},
//	{"FBA_1", GIF_A_D_REG_FBA_1},
//	{"FBA_2", GIF_A_D_REG_FBA_2},
//	{"FRAME_1", GIF_A_D_REG_FRAME_1},
//	{"FRAME_2", GIF_A_D_REG_FRAME_2},
//	{"ZBUF_1", GIF_A_D_REG_ZBUF_1},
//	{"ZBUF_2", GIF_A_D_REG_ZBUF_2},
//	{"BITBLTBUF", GIF_A_D_REG_BITBLTBUF},
//	{"TRXPOS", GIF_A_D_REG_TRXPOS},
//	{"TRXREG", GIF_A_D_REG_TRXREG},
//	{"TRXDIR", GIF_A_D_REG_TRXDIR},
//	{"HWREG", GIF_A_D_REG_HWREG},
//	{"SIGNAL", GIF_A_D_REG_SIGNAL},
//	{"FINISH", GIF_A_D_REG_FINISH},
//	{"LABEL", GIF_A_D_REG_LABEL},
//};
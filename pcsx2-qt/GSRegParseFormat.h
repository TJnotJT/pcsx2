#pragma once

#include "GS.h"
#include "GS/GSRegs.h"

struct GSRegField
{
	std::string name;
	u32 start_bit;
	u32 end_bit;
	enum Format
	{
		// Common formats
		FormatFrameAddress, // Word address / 2048
		FormatAddress, // Word address / 64
		FormatBufferWidth, // Pixels / 64
		FormatUnsigned, // Unsigned integer
		FormatUnsigned_4, // 4 bits fractional
		FormatSigned, // 1 bit sign
		FormatSigned_7_4, // 1 bit sign, 7 bits integer, 4 bits fractional
		FormatFloat, // 32 bits floating point
		FormatColor

		// Alpha
		FormatAlphaABD,
		FormatAlphaC,

		// CLAMP
		FormatWrapMode,

		// COLCLAMP
		FormatCLAMP,

		// DIMX
		FormatSigned_2_1,

		// DTHE
		FormatDTHE,

		// FBA
		FormatFBA,

		// Pixel formats
		FormatPSMC, // Color
		FormatPSMZ, // Depth

		// PABE
		FormatPABE,

		// PRIM
		FormatPRIM,
		FormatIIP,
		FormatTME,
		FormatFGE,
		FormatABE,
		FormatAA1,
		FormatFST,
		FormatFIX,

		// TEXA
		FormatAEM, // Alpha Enable Mode

		// TEXO, TEX2
		FormatLog2Size,
		FormatTCC,
		FormatTFX,
		FormatCSM,
		FormatTexelOffset,

		// TEX1
		FormatLCM,
		FormatMMAG,
		FormatMMIN,
		FormatMTBA,
		FormatL,
		FormatSigned_7_4, // K

		// TEXCLUT
		FormatTexelOffset,

		// TRXDIR
		FormatXDIR,

		// TRXPOS
		FormatDir,

		// Packed XYZF2
		FormatADC,

		// Packed A+D
		FormatA_D,
		FormatPackedRegID, // Used for A+D ADDR field
	} format;
};


// Save duplication when multiple registers can be described by the
// same fields (e.g. CLAMP_1 and CLAMP_2).
struct GSRegInfoCompressed
{
	const std::vector<std::tuple<u32, std::string>> id_name;
	const std::map<std::string, GSRegField> fields;
};


struct GSRegInfo
{
	const u32 id;
	const std::string name;
	const std::vector<GSRegField> fields;

	GSRegInfo(const u32 id, const std::string& name, const std::vector<GSRegField>& fields)
		: id(id)
		, name(name)
		, fields(fields)
	{
	}
};


extern GSRegInfoCompressed regInfoCompressed[];
extern GSRegInfo gsRegInfoIdMap[GIF_A_D_REG_ZBUF_2 + 1];    // maps IDs to descriptors
extern std::map<std::string, GSRegInfo> gsRegInfoNameMap[]; // maps names to descriptors

std::string GSRegGetName(u32 id);
void GSRegDecodeFields(u32 id, u64 data, std::vector<std::tuple<std::string, u64>> fields);
std::string GSRegFieldDoFormat(u32 id, u64 bits, GSRegField::Format fmt);
std::string GSRegFormat(u32 id, u64 data);
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
		FrameAddress, // Word address / 2048
		Address, // Word address / 64
		BufferWidth, // Pixels / 64
		Unsigned, // Unsigned integer
		Unsigned_4, // 4 bits fractional
		Signed, // 1 bit sign
		Float, // 32 bits floating point
		Color,
		TexelOffset, // Offset / 16

		// Alpha
		AlphaABD,
		AlphaC,

		// CLAMP
		WrapMode,

		// COLCLAMP
		CLAMP,

		// DIMX
		Signed_2_1,

		// DTHE
		DTHE,

		// FBA
		FBA,

		// Pixel formats
		PSMC, // Color
		PSMZ, // Depth

		// PABE
		PABE,

		// PRIM
		PRIM,
		IIP,
		TME,
		FGE,
		ABE,
		AA1,
		FST,
		FIX,

		// TEXA
		AEM, // Alpha Enable Mode

		// TEXO, TEX2
		Log2Size,
		TCC,
		TFX,
		CSM,

		// TEX1
		LCM,
		MMAG,
		MMIN,
		MTBA,
		L,
		Signed_7_4, // K, 1 bit sign, 7 bits integer, 4 bits fractional

		// TRXDIR
		XDIR,

		// TRXPOS
		DIR,

		// ZBUF
		ZMSK,

		// Packed XYZF2
		ADC,

		// Packed A+D
		A_D,
		PackedRegID, // Used for A+D ADDR field
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
	u32 id;
	std::string name;
	std::map<std::string, GSRegField> fields;
};

std::string GSRegGetName(u32 id);
bool GSRegEncodeFields(u32 id, const std::map<std::string, u64>& field_vals, u64* data);
bool GSPackedRegEndcodeFields(u32 id, const std::map<std::string, u64>& field_vals, u64* data);
bool GSRegDecodeFields(u32 id, u64* data, std::map<std::string, u64>* field_vals);
bool GSPackedRegDecodeFields(u32 id, u64* data, std::map<std::string, u64>* field_vals);

std::string GSRegFieldDoFormat(u32 id, u64 bits, GSRegField::Format fmt);
std::string GSRegFormat(u32 id, u64 data);
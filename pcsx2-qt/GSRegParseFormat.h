#pragma once

#include "GS.h"
#include "GS/GSRegs.h"

struct GSRegField
{
	enum class ID
	{
		ALPHA_A,
		ALPHA_B,
		ALPHA_C,
		ALPHA_D,
		ALPHA_FIX,
		BITBLTBUF_SBP,
		BITBLTBUF_SBW,
		BITBLTBUF_SPSM,
		BITBLTBUF_DBP,
		BITBLTBUF_DBW,
		BITBLTBUF_DPSM,
		CLAMP_WMS,
		CLAMP_WMT,
		CLAMP_MINU,
		CLAMP_MAXU,
		CLAMP_MINV,
		CLAMP_MAXV,
		COLCLAMP_CLAMP,
		DIMX_DM00,
		DIMX_DM01,
		DIMX_DM02,
		DIMX_DM03,
		DIMX_DM10,
		DIMX_DM11,
		DIMX_DM12,
		DIMX_DM13,
		DIMX_DM20,
		DIMX_DM21,
		DIMX_DM22,
		DIMX_DM23,
		DIMX_DM30,
		DIMX_DM31,
		DIMX_DM32,
		DIMX_DM33,
		DTHE_DTHE,
		FBA_FBA,
		FINISH_FINISH,
		FOG_F,
		FOGCOL_FCR,
		FOGCOL_FCG,
		FOGCOL_FCB,
		FRAME_FBP,
		FRAME_FBW,
		FRAME_PSM,
		FRAME_FBMSK,
		HWREG_DATA,
		LABEL_ID,
		LABEL_IDMSK,
		MIPTBP1_TBP1,
		MIPTBP1_TBW1,
		MIPTBP1_TBP2,
		MIPTBP1_TBW2,
		MIPTBP1_TBP3,
		MIPTBP1_TBW3,
		MIPTBP2_TBP4,
		MIPTBP2_TBW4,
		MIPTBP2_TBP5,
		MIPTBP2_TBW5,
		MIPTBP2_TBP6,
		MIPTBP2_TBW6,
		PABE_PABE,
		PRIM_PRIM,
		PRIM_IIP,
		PRIM_TME,
		PRIM_FGE,
		PRIM_ABE,
		PRIM_AA1,
		PRIM_FST,
		PRIM_CTXT,
		PRIM_FIX,
		PRMODECONT_AC,
		RGBAQ_R,
		RGBAQ_G,
		RGBAQ_B,
		RGBAQ_A,
		RGBAQ_Q,
		SCANMSK_MSK,
		ST_S,
		ST_T,
		TEST_ATE,
		TEST_ATST,
		TEST_AREF,
		TEST_AFAIL,
		TEST_DATE,
		TEST_DATM,
		TEST_ZTE,
		TEST_ZTST,
		TEX0_TBP0,
		TEX0_TBPW,
		TEX0_PSM,
		TEX0_TW,
		TEX0_TH,
		TEX0_TCC,
		TEX0_TFX,
		TEX0_CBP,
		TEX0_CPSM,
		TEX0_CSM,
		TEX0_CSA,
		TEX0_CLD,
		TEX1_LCM,
		TEX1_MXL,
		TEX1_MMAG,
		TEX1_MMIN,
		TEX1_MTBA,
		TEX1_L,
		TEX1_K,
		TEX2_PSM,
		TEX2_CBP,
		TEX2_CPSM,
		TEX2_CSM,
		TEX2_CSA,
		TEX2_CLD,
		TEXA_TA0,
		TEXA_AEM,
		TEXA_TA1,
		TEXCLUT_CBW,
		TEXCLUT_COU,
		TEXCLUT_COV,
		TEXFLUSH_DATA,
		TRXDIR_XDIR,
		TRXPOS_SSAX,
		TRXPOS_SSAY,
		TRXPOS_DSAX,
		TRXPOS_DSAY,
		TRXPOS_DIR,
		TRXREG_RRW,
		TRXREG_RRH,
		UV_U,
		UV_V,
		XYOFFSET_OFX,
		XYOFFSET_OFY,
		XYZF2_X,
		XYZF2_Y,
		XYZF2_Z,
		XYZF2_F,
		XYZ2_X,
		XYZ2_Y,
		XYZ2_Z,
		ZBUF_ZBP,
		ZBUF_PSM,
		ZBUF_ZMSK,

		PACKED_PRIM_PRIM,
		PACKED_PRIM_IIP,
		PACKED_PRIM_TME,
		PACKED_PRIM_FGE,
		PACKED_PRIM_ABE,
		PACKED_PRIM_AA1,
		PACKED_PRIM_FST,
		PACKED_PRIM_CTXT,
		PACKED_PRIM_FIX,
		PACKED_RGBA_R,
		PACKED_RGBA_G,
		PACKED_RGBA_B,
		PACKED_RGBA_A,
		PACKED_STQ_S,
		PACKED_STQ_T,
		PACKED_STQ_Q,
		PACKED_UV_U,
		PACKED_UV_V,
		PACKED_XYZF2_X,
		PACKED_XYZF2_Y,
		PACKED_XYZF2_Z,
		PACKED_XYZF2_F,
		PACKED_XYZF2_ADC,
		PACKED_XYZ2_X,
		PACKED_XYZ2_Y,
		PACKED_XYZ2_Z,
		PACKED_XYZ2_ADC,
		PACKED_FOG_F,
		PACKED_AD_DATA,
		PACKED_AD_ADDR,
		PACKED_NOP_DATA,
	};
	enum class Format
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
		PSM, // Pixel formats

		// Alpha
		ALPHA_ABD,
		ALPHA_C,

		// CLAMP
		CLAMP_WM,

		// COLCLAMP
		COLCLAMP_CLAMP,

		// DIMX
		Signed_2_1,

		// DTHE
		DTHE_DTHE,

		// FBA
		FBA_FBA,

		// PABE
		PABE_PABE,

		// PRIM
		PRIM_PRIM,
		PRIM_IIP,
		PRIM_TME,
		PRIM_FGE,
		PRIM_ABE,
		PRIM_AA1,
		PRIM_FST,
		PRIM_FIX,

		// PRMODECONT
		PRMODECONT_AC,

		// SCANMSK
		SCANMSK_MSK,

		// TEST
		TEST_ATST,
		TEST_AFAIL,
		TEST_DATM,
		TEST_ZTST,

		// TEXA
		TEXA_AEM,

		// TEXO, TEX2
		TEX0_Log2Size,
		TEX0_TCC,
		TEX0_TFX,
		TEX0_CPSM,
		TEX0_CSM,

		// TEX1
		TEX1_LCM,
		TEX1_MMAG,
		TEX1_MMIN,
		TEX1_MTBA,
		TEX1_L,
		Signed_7_4, // K, 1 bit sign, 7 bits integer, 4 bits fractional

		// TRXDIR
		TRXDIR_XDIR,

		// TRXPOS
		TRXPOS_DIR,

		// ZBUF
		ZBUF_ZMSK,
		ZBUF_PSM,

		// Packed XYZF2
		PACKED_XYZ2_ADC,

		// Packed A+D
		PACKED_AD_ADDR,
		PACKED_AD_DATA,
	};
	ID id;
	const char* name;
	u32 start_bit;
	u32 end_bit;

	Format format;
};

struct GSRegInfo
{
	u32 id;
	const char* name;
	std::map<GSRegField::ID, GSRegField> fields;
};

std::string GSRegGetName(u32 id);
std::string GSPackedRegGetName(u32 id);
bool GSRegEncodeFields(u32 id, const std::map<GSRegField::ID, u64>& field_vals, u8* data);
bool GSPackedRegEndcodeFields(u32 id, const std::map<GSRegField::ID, u64>& field_vals, u8* data);
bool GSRegDecodeFields(u32 id, const u8* data, std::map<GSRegField::ID, u64>* field_vals);
bool GSPackedRegDecodeFields(u32 id, const u8* data, std::map<GSRegField::ID, u64>* field_vals);
std::string GSRegFormatField(GSRegField::Format format, u64 data);
std::string GSRegFormat(u32 id, u64 data);
#include <sstream>
#include "GSRegParseFormat.h"

using CompressedRegIdList = std::vector<std::tuple<u32, const char*>>;
using CompressedFieldList = std::vector<std::tuple<GSRegField::ID, const char*, u32, u32, GSRegField::Format>>;

std::vector<std::tuple<CompressedRegIdList, CompressedFieldList>> gsRegInfoCompressed =
{
	{{{GIF_A_D_REG_ALPHA_1, "ALPHA_1"}, {GIF_A_D_REG_ALPHA_2, "ALPHA_2"}}, {
		{GSRegField::ID::ALPHA_A, "A", 0, 2, GSRegField::Format::ALPHA_ABD},
		{GSRegField::ID::ALPHA_B, "B", 2, 4, GSRegField::Format::ALPHA_ABD},
		{GSRegField::ID::ALPHA_C, "C", 4, 6, GSRegField::Format::ALPHA_C},
		{GSRegField::ID::ALPHA_D, "D", 6, 8, GSRegField::Format::ALPHA_ABD},
		{GSRegField::ID::ALPHA_FIX, "FIX", 32, 40, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_BITBLTBUF, "BITBLTBUF"}}, {
		{GSRegField::ID::BITBLTBUF_SBP, "SBP", 0, 14, GSRegField::Format::Address},
		{GSRegField::ID::BITBLTBUF_SBW, "SBW", 16, 22, GSRegField::Format::BufferWidth},
		{GSRegField::ID::BITBLTBUF_SPSM, "SPSM", 24, 30, GSRegField::Format::PSM},
		{GSRegField::ID::BITBLTBUF_DBP, "DBP", 32, 46, GSRegField::Format::Address},
		{GSRegField::ID::BITBLTBUF_DBW, "DBW", 48, 54, GSRegField::Format::BufferWidth},
		{GSRegField::ID::BITBLTBUF_DPSM, "DPSM", 56, 62, GSRegField::Format::PSM},
	}},
	{{{GIF_A_D_REG_CLAMP_1, "CLAMP_1"}, {GIF_A_D_REG_CLAMP_2, "CLAMP_2"}}, {
		{GSRegField::ID::CLAMP_WMS, "WMS", 0, 2, GSRegField::Format::CLAMP_WM},
		{GSRegField::ID::CLAMP_WMT, "WMT", 2, 3, GSRegField::Format::CLAMP_WM},
		{GSRegField::ID::CLAMP_MINU, "MINU", 4, 14, GSRegField::Format::Unsigned},
		{GSRegField::ID::CLAMP_MAXU, "MAXU", 14, 24, GSRegField::Format::Unsigned},
		{GSRegField::ID::CLAMP_MINV, "MINV", 24, 34, GSRegField::Format::Unsigned},
		{GSRegField::ID::CLAMP_MAXV, "MAXV", 34, 44, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_COLCLAMP, "COLCLAMP"}}, {
		{GSRegField::ID::COLCLAMP_CLAMP, "CLAMP", 0, 1, GSRegField::Format::COLCLAMP_CLAMP},
	}},
	{{{GIF_A_D_REG_DIMX, "DIMX"}}, {
		{GSRegField::ID::DIMX_DM00, "DM00", 0, 3, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM01, "DM01", 4, 7, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM02, "DM02", 8, 11, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM03, "DM03", 12, 15, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM10, "DM10", 16, 19, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM11, "DM11", 20, 23, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM12, "DM12", 24, 27, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM13, "DM13", 28, 31, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM20, "DM20", 32, 35, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM21, "DM21", 36, 39, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM22, "DM22", 40, 43, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM23, "DM23", 44, 47, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM30, "DM30", 48, 51, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM31, "DM31", 52, 55, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM32, "DM32", 56, 59, GSRegField::Format::Signed_2_1},
		{GSRegField::ID::DIMX_DM33, "DM33", 60, 63, GSRegField::Format::Signed_2_1},
	}},
	{{{GIF_A_D_REG_DTHE, "DTHE"}}, {
		{GSRegField::ID::DTHE_DTHE, "DTHE", 0, 1, GSRegField::Format::DTHE_DTHE},
	}},
	{{{GIF_A_D_REG_FBA_1, "FBA_1"}, {GIF_A_D_REG_FBA_2, "FBA_2"}}, {
		{GSRegField::ID::FBA_FBA, "FBA", 0, 1, GSRegField::Format::FBA_FBA},
	}},
	{{{GIF_A_D_REG_FINISH, "FINISH"}}, {
		{GSRegField::ID::FINISH_FINISH, "FINISH", 0, 1, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_FOG, "FOG"}}, {
		{GSRegField::ID::FOG_F, "F", 0, 8, GSRegField::Format::Color},
	}},
	{{{GIF_A_D_REG_FOGCOL, "FOGCOL"}}, {
		{GSRegField::ID::FOGCOL_FCR, "FCR", 0, 8, GSRegField::Format::Color},
		{GSRegField::ID::FOGCOL_FCG, "FCG", 8, 16, GSRegField::Format::Color},
		{GSRegField::ID::FOGCOL_FCB, "FCB", 16, 24, GSRegField::Format::Color},
	}},
	{{{GIF_A_D_REG_FRAME_1, "FRAME_1"}, {GIF_A_D_REG_FRAME_2, "FRAME_2"}}, {
		{GSRegField::ID::FRAME_FBP, "FBP", 0, 9, GSRegField::Format::Address},
		{GSRegField::ID::FRAME_FBW, "FBW", 16, 22, GSRegField::Format::BufferWidth},
		{GSRegField::ID::FRAME_PSM, "PSM", 24, 30, GSRegField::Format::PSM},
		{GSRegField::ID::FRAME_FBMSK, "FBMSK", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_HWREG, "HWREG"}}, {
		{GSRegField::ID::HWREG_DATA, "DATA", 0, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_LABEL, "LABEL"}}, {
		{GSRegField::ID::LABEL_ID, "ID", 0, 32, GSRegField::Format::Unsigned},
		{GSRegField::ID::LABEL_IDMSK, "IDMSK", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_MIPTBP1_1, "MIPTBP1_1"}, {GIF_A_D_REG_MIPTBP1_2, "MIPTBP1_2"}}, {
		{GSRegField::ID::MIPTBP1_TBP1, "TBP1", 0, 14, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP1_TBW1, "TBW1", 14, 20, GSRegField::Format::BufferWidth},
		{GSRegField::ID::MIPTBP1_TBP2, "TBP2", 20, 34, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP1_TBW2, "TBW2", 34, 40, GSRegField::Format::BufferWidth},
		{GSRegField::ID::MIPTBP1_TBP3, "TBP3", 40, 54, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP1_TBW3, "TBW3", 54, 60, GSRegField::Format::BufferWidth},
	}},
	{{{GIF_A_D_REG_MIPTBP2_1, "MIPTBP2_1"}, {GIF_A_D_REG_MIPTBP2_2, "MIPTBP2_2"}}, {
		{GSRegField::ID::MIPTBP2_TBP4, "TBP4", 0, 14, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP2_TBW4, "TBW4", 14, 20, GSRegField::Format::BufferWidth},
		{GSRegField::ID::MIPTBP2_TBP5, "TBP5", 20, 34, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP2_TBW5, "TBW5", 34, 40, GSRegField::Format::BufferWidth},
		{GSRegField::ID::MIPTBP2_TBP6, "TBP6", 40, 54, GSRegField::Format::Address},
		{GSRegField::ID::MIPTBP2_TBW6, "TBW6", 54, 60, GSRegField::Format::BufferWidth},
	}},
	{{{GIF_A_D_REG_PABE, "PABE"}}, {
		{GSRegField::ID::PABE_PABE, "PABE", 0, 1, GSRegField::Format::PABE_PABE},
	}},
	{{{GIF_A_D_REG_PRIM, "PRIM"}, {GIF_A_D_REG_PRIM, "PRMODE"}}, {
		{GSRegField::ID::PRIM_PRIM, "PRIM", 0, 3, GSRegField::Format::PRIM_PRIM},
		{GSRegField::ID::PRIM_IIP, "IIP", 3, 4, GSRegField::Format::PRIM_IIP},
		{GSRegField::ID::PRIM_TME, "TME", 4, 5, GSRegField::Format::PRIM_TME},
		{GSRegField::ID::PRIM_FGE, "FGE", 5, 6, GSRegField::Format::PRIM_FGE},
		{GSRegField::ID::PRIM_ABE, "ABE", 6, 7, GSRegField::Format::PRIM_ABE},
		{GSRegField::ID::PRIM_AA1, "AA1", 7, 8, GSRegField::Format::PRIM_AA1},
		{GSRegField::ID::PRIM_FST, "FST", 8, 9, GSRegField::Format::PRIM_FST},
		{GSRegField::ID::PRIM_CTXT, "CTXT", 9, 10, GSRegField::Format::Unsigned},
		{GSRegField::ID::PRIM_FIX, "FIX", 10, 11, GSRegField::Format::PRIM_FIX},
	}},
	{{{GIF_A_D_REG_PRMODECONT, "PRMODECONT"}}, {
		{GSRegField::ID::PRMODECONT_AC, "PRMODE", 0, 1, GSRegField::Format::PRMODECONT_AC},
	}},
	{{{GIF_A_D_REG_RGBAQ, "RGBAQ"}}, {
		{GSRegField::ID::RGBAQ_R, "R", 0, 8, GSRegField::Format::Color},
		{GSRegField::ID::RGBAQ_G, "G", 8, 16, GSRegField::Format::Color},
		{GSRegField::ID::RGBAQ_B, "B", 16, 24, GSRegField::Format::Color},
		{GSRegField::ID::RGBAQ_A, "A", 24, 32, GSRegField::Format::Unsigned},
		{GSRegField::ID::RGBAQ_Q, "Q", 32, 64, GSRegField::Format::Float},
	}},
	{{{GIF_A_D_REG_SCANMSK, "SCANMSK"}}, {
		{GSRegField::ID::SCANMSK_MSK, "MSK", 0, 2, GSRegField::Format::SCANMSK_MSK},
	}},
	{{{GIF_A_D_REG_ST, "ST"}}, {
		{GSRegField::ID::ST_S, "S", 0, 32, GSRegField::Format::Float},
		{GSRegField::ID::ST_T, "T", 32, 64, GSRegField::Format::Float},
	}},
	{{{GIF_A_D_REG_TEST_1, "TEST_1"}, {GIF_A_D_REG_TEST_2, "TEST_2"}}, {
		{GSRegField::ID::TEST_ATE, "ATE", 0, 1, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEST_ATST, "ATST", 1, 4, GSRegField::Format::TEST_ATST},
		{GSRegField::ID::TEST_AREF, "AREF", 4, 12, GSRegField::Format::Unsigned}, 
		{GSRegField::ID::TEST_AFAIL, "AFAIL", 12, 14, GSRegField::Format::TEST_AFAIL},
		{GSRegField::ID::TEST_DATE, "DATE", 14, 15, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEST_DATM, "DATM", 15, 16, GSRegField::Format::TEST_DATM},
		{GSRegField::ID::TEST_ZTE, "ZTE", 16, 17, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEST_ZTST, "ZTST", 17, 20, GSRegField::Format::TEST_ZTST},
	}},
	{{{GIF_A_D_REG_TEX0_1, "TEX0_1"}, {GIF_A_D_REG_TEX0_2, "TEX0_2"}}, {
		{GSRegField::ID::TEX0_TBP0, "TBP0", 0, 14, GSRegField::Format::Address},
		{GSRegField::ID::TEX0_TBPW, "TBPW", 14, 20, GSRegField::Format::BufferWidth},
		{GSRegField::ID::TEX0_PSM, "PSM", 20, 26, GSRegField::Format::PSM},
		{GSRegField::ID::TEX0_TW, "TW", 26, 30, GSRegField::Format::TEX0_Log2Size},
		{GSRegField::ID::TEX0_TH, "TH", 30, 34, GSRegField::Format::TEX0_Log2Size},
		{GSRegField::ID::TEX0_TCC, "TCC", 34, 36, GSRegField::Format::TEX0_TCC},
		{GSRegField::ID::TEX0_TFX, "TFX", 35, 37, GSRegField::Format::TEX0_TFX},
		{GSRegField::ID::TEX0_CBP, "CBP", 37, 51, GSRegField::Format::Address},
		{GSRegField::ID::TEX0_CPSM, "CPSM", 51, 55, GSRegField::Format::TEX0_CPSM},
		{GSRegField::ID::TEX0_CSM, "CSM", 55, 56, GSRegField::Format::TEX0_CSM},
		{GSRegField::ID::TEX0_CSA, "CSA", 56, 61, GSRegField::Format::TexelOffset},
		{GSRegField::ID::TEX0_CLD, "CLD", 61, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEX1_1, "TEX1_1"}, {GIF_A_D_REG_TEX1_2, "TEX1_2"}}, {
		{GSRegField::ID::TEX1_LCM, "LCM", 0, 2, GSRegField::Format::TEX1_LCM},
		{GSRegField::ID::TEX1_MXL, "MXL", 2, 5, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEX1_MMAG, "MMAG", 5, 6, GSRegField::Format::TEX1_MMAG},
		{GSRegField::ID::TEX1_MMIN, "MMIN", 6, 9, GSRegField::Format::TEX1_MMIN},
		{GSRegField::ID::TEX1_MTBA, "MTBA", 9, 10, GSRegField::Format::TEX1_MTBA},
		{GSRegField::ID::TEX1_L, "L", 19, 21, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEX1_K, "K", 32, 44, GSRegField::Format::Signed_7_4},
	}},
	{{{GIF_A_D_REG_TEX2_1, "TEX2_1"}, {GIF_A_D_REG_TEX2_2, "TEX2_2"}}, {
		{GSRegField::ID::TEX2_PSM, "PSM", 20, 26, GSRegField::Format::PSM},
		{GSRegField::ID::TEX2_CBP, "CBP", 37, 51, GSRegField::Format::Address},
		{GSRegField::ID::TEX2_CPSM, "CPSM", 51, 55, GSRegField::Format::TEX0_CPSM},
		{GSRegField::ID::TEX2_CSM, "CSM", 55, 56, GSRegField::Format::TEX0_CSM},
		{GSRegField::ID::TEX2_CSA, "CSA", 56, 60, GSRegField::Format::TexelOffset},
		{GSRegField::ID::TEX2_CLD, "CLD", 61, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXA, "TEXA"}}, {
		{GSRegField::ID::TEXA_TA0, "TA0", 0, 8, GSRegField::Format::Unsigned},
		{GSRegField::ID::TEXA_AEM, "AEM", 15, 16, GSRegField::Format::TEXA_AEM},
		{GSRegField::ID::TEXA_TA1, "TA1", 32, 40, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXCLUT, "TEXCLUT"}}, {
		{GSRegField::ID::TEXCLUT_CBW, "CBW", 0, 8, GSRegField::Format::BufferWidth},
		{GSRegField::ID::TEXCLUT_COU, "COU", 8, 24, GSRegField::Format::TexelOffset},
		{GSRegField::ID::TEXCLUT_COV, "COV", 12, 22, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TEXFLUSH, "TEXFLUSH"}}, {
		{GSRegField::ID::TEXFLUSH_DATA, "DATA", 0, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_TRXDIR, "TRXDIR"}}, {
		{GSRegField::ID::TRXDIR_XDIR, "XDIR", 0, 2, GSRegField::Format::TRXDIR_XDIR},
	}},
	{{{GIF_A_D_REG_TRXPOS, "TRXPOS"}}, {
		{GSRegField::ID::TRXPOS_SSAX, "SSAX", 0, 11, GSRegField::Format::Unsigned},
		{GSRegField::ID::TRXPOS_SSAY, "SSAY", 16, 27, GSRegField::Format::Unsigned},
		{GSRegField::ID::TRXPOS_DSAX, "DSAX", 32, 43, GSRegField::Format::Unsigned},
		{GSRegField::ID::TRXPOS_DSAY, "DSAY", 48, 59, GSRegField::Format::Unsigned},
		{GSRegField::ID::TRXPOS_DIR, "DIR", 59, 61, GSRegField::Format::TRXPOS_DIR},
	}},
	{{{GIF_A_D_REG_TRXREG, "TRXREG"}}, {
		{GSRegField::ID::TRXREG_RRW, "RRW", 0, 11, GSRegField::Format::Unsigned},
		{GSRegField::ID::TRXREG_RRH, "RRH", 32, 44, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_UV, "UV"}}, {
		{GSRegField::ID::UV_U, "U", 0, 14, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::UV_V, "V", 16, 30, GSRegField::Format::Unsigned_4},
	}},
	{{{GIF_A_D_REG_XYOFFSET_1, "XYOFFSET_1"}, {GIF_A_D_REG_XYOFFSET_2, "XYOFFSET_2"}}, {
		{GSRegField::ID::XYOFFSET_OFX, "OFX", 0, 16, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::XYOFFSET_OFY, "OFY", 32, 48, GSRegField::Format::Unsigned_4},
	}},
	{{{GIF_A_D_REG_XYZF2, "XYZF2"}, {GIF_A_D_REG_XYZF3, "XYZF3"}}, {
		{GSRegField::ID::XYZF2_X, "X", 0, 16, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::XYZF2_Y, "Y", 16, 32, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::XYZF2_Z, "Z", 32, 56, GSRegField::Format::Unsigned},
		{GSRegField::ID::XYZF2_F, "F", 56, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_XYZ2, "XYZ2"}, {GIF_A_D_REG_XYZ3, "XYZ3"}}, {
		{GSRegField::ID::XYZ2_X, "X", 0, 16, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::XYZ2_Y, "Y", 16, 32, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::XYZ2_Z, "Z", 32, 64, GSRegField::Format::Unsigned},
	}},
	{{{GIF_A_D_REG_ZBUF_1, "ZBUF_1"}, {GIF_A_D_REG_ZBUF_2, "ZBUF_2"}}, {
		{GSRegField::ID::ZBUF_ZBP, "ZBP", 0, 10, GSRegField::Format::Address},
		{GSRegField::ID::ZBUF_PSM, "PSM", 24, 28, GSRegField::Format::ZBUF_PSM},
		{GSRegField::ID::ZBUF_ZMSK, "ZMSK", 32, 33, GSRegField::Format::ZBUF_ZMSK},
	}},
};

std::vector<std::tuple<CompressedRegIdList,	CompressedFieldList>> gsPackedRegInfoCompressed =
{
	{{{GIF_REG_PRIM, "PRIM"}}, {
		{GSRegField::ID::PACKED_PRIM_PRIM, "PRIM", 0, 3, GSRegField::Format::PRIM_PRIM},
		{GSRegField::ID::PACKED_PRIM_IIP, "IIP", 3, 4, GSRegField::Format::PRIM_IIP},
		{GSRegField::ID::PACKED_PRIM_TME, "TME", 4, 5, GSRegField::Format::PRIM_TME},
		{GSRegField::ID::PACKED_PRIM_FGE, "FGE", 5, 6, GSRegField::Format::PRIM_FGE},
		{GSRegField::ID::PACKED_PRIM_ABE, "ABE", 6, 7, GSRegField::Format::PRIM_ABE},
		{GSRegField::ID::PACKED_PRIM_AA1, "AA1", 7, 8, GSRegField::Format::PRIM_AA1},
		{GSRegField::ID::PACKED_PRIM_FST, "FST", 8, 9, GSRegField::Format::PRIM_FST},
		{GSRegField::ID::PACKED_PRIM_CTXT, "CTXT", 9, 10, GSRegField::Format::Unsigned},
		{GSRegField::ID::PACKED_PRIM_FIX, "FIX", 10, 11, GSRegField::Format::PRIM_FIX},
	}},
	{{{GIF_REG_RGBA, "RGBA"}}, {
		{GSRegField::ID::PACKED_RGBA_R, "R", 0, 8, GSRegField::Format::Color},
		{GSRegField::ID::PACKED_RGBA_G, "G", 32, 40, GSRegField::Format::Color},
		{GSRegField::ID::PACKED_RGBA_B, "B", 64, 72, GSRegField::Format::Color},
		{GSRegField::ID::PACKED_RGBA_A, "A", 96, 104, GSRegField::Format::Color},
	}},
	{{{GIF_REG_STQ, "STQ"}}, {
		{GSRegField::ID::PACKED_STQ_S, "S", 0, 32, GSRegField::Format::Float},
		{GSRegField::ID::PACKED_STQ_T, "T", 32, 64, GSRegField::Format::Float},
		{GSRegField::ID::PACKED_STQ_Q, "Q", 64, 96, GSRegField::Format::Float},
	}},
	{{{GIF_REG_UV, "UV"}}, {
		{GSRegField::ID::PACKED_UV_U, "U", 0, 14, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::PACKED_UV_V, "V", 32, 46, GSRegField::Format::Unsigned_4},
	}},
	{{{GIF_REG_XYZF2, "XYZF2"}}, {
		{GSRegField::ID::PACKED_XYZF2_X, "X", 0, 16, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::PACKED_XYZF2_Y, "Y", 32, 48, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::PACKED_XYZF2_Z, "Z", 68, 92, GSRegField::Format::Unsigned},
		{GSRegField::ID::PACKED_XYZF2_F, "F", 100, 108, GSRegField::Format::Color},
		{GSRegField::ID::PACKED_XYZF2_ADC, "ADC", 111, 112, GSRegField::Format::PACKED_XYZ2_ADC},
	}},
	{{{GIF_REG_XYZ2, "XYZ2"}}, {
		{GSRegField::ID::PACKED_XYZ2_X, "X", 0, 16, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::PACKED_XYZ2_Y, "Y", 32, 48, GSRegField::Format::Unsigned_4},
		{GSRegField::ID::PACKED_XYZ2_Z, "Z", 64, 96, GSRegField::Format::Unsigned},
		{GSRegField::ID::PACKED_XYZ2_ADC, "ADC", 111, 112, GSRegField::Format::PACKED_XYZ2_ADC},
	}},
	{{{GIF_REG_FOG, "FOG"}}, {
		{GSRegField::ID::PACKED_FOG_F, "F", 100, 108, GSRegField::Format::Color},
	}},
	{{{GIF_REG_A_D, "A_D"}}, {
		{GSRegField::ID::PACKED_AD_DATA, "DATA", 0, 64, GSRegField::Format::PACKED_AD_DATA},
		{GSRegField::ID::PACKED_AD_ADDR, "ADDR", 64, 96, GSRegField::Format::PACKED_AD_ADDR},
	}},
	{{{GIF_REG_NOP, "NOP"}}, {
		{GSRegField::ID::PACKED_NOP_DATA, "DATA", 0, 64, GSRegField::Format::Unsigned},
	}},
};

std::map<GSRegField::Format, std::map<u64, const char*>> gsRegFieldValues = {
	{GSRegField::Format::PSM, {
		{PSMCT32, "PSMCT32"}, {PSMCT24, "PSMCT24"}, {PSMCT16, "PSMCT16"}, {PSMCT16S, "PSMCT16S"},
		{PSMT8, "PSMT8"}, {PSMT4, "PSMT4"}, {PSMT8H, "PSMT8H"}, {PSMT4HL, "PSMT4HL"}, {PSMT4HH, "PSMT4HH"},
		{PSMZ32, "PSMZ32"}, {PSMZ24, "PSMZ24"}, {PSMZ16, "PSMZ16"}, {PSMZ16S, "PSMZ16S"}}},
	{GSRegField::Format::ALPHA_ABD, {{0, "Cs"}, {1, "Cd"}, {2, "0"}}},
	{GSRegField::Format::ALPHA_C, {{0, "As"}, {1, "Ad"}, {2, "FIX"}}},
	{GSRegField::Format::CLAMP_WM, {{0, "REPEAT"}, {1, "CLAMP"}, {2, "REGION_CLAMP"}, {3, "REGION_REPEAT"}}},
	{GSRegField::Format::COLCLAMP_CLAMP, {{0, "MASK"}, {1, "CLAMP"}}},
	{GSRegField::Format::FBA_FBA, {{0, "RGBA32"}, {1, "RGBA16"}}},
	{GSRegField::Format::PRIM_PRIM, {{0, "POINT"}, {1, "LINE"}, {2, "LINESTRIP"}, {3, "TRIANGLE"}, {4, "TRIANGLESTRIP"}, {5, "TRIANGLEFAN"}, {6, "SPRITE"}, {7, "PROHIBITED"}}},
	{GSRegField::Format::PRIM_IIP, {{0, "FLAT"}, {1, "GOURAUD"}}},
	{GSRegField::Format::PRIM_FST, {{0, "STQ"}, {1, "UV"}}},
	{GSRegField::Format::PRMODECONT_AC, {{0, "PRMODE"}, {1, "PRIM"}}},
	{GSRegField::Format::SCANMSK_MSK, {{0, "NORMAL"}, {1, "RESERVED"}, {2, "EVEN_PROHIBITED"}, {3, "ODD_PROHIBITED"}}},
	{GSRegField::Format::TEST_ATST, {{0, "NEVER"}, {1, "ALWAYS"}, {2, "LESS"}, {3, "LEQUAL"}, {4, "EQUAL"}, {5, "GEQUAL"}, {6, "GREATER"}, {7, "NOTEQUAL"}}},
	{GSRegField::Format::TEST_AFAIL, {{0, "KEEP"}, {1, "FB_ONLY"}, {2, "ZB_ONLY"}, {3, "RGB_ONLY"}}},
	{GSRegField::Format::TEST_DATM, {{0, "0PASS"}, {1, "1PASS"}}},
	{GSRegField::Format::TEST_ZTST, {{0, "NEVER"}, {1, "ALWAYS"}, {2, "GEQUAL"}, {3, "GREATER"}}},
	{GSRegField::Format::TEX0_TCC, {{0, "RGB"}, {1, "RGBA"}}},
	{GSRegField::Format::TEX0_TFX, {{0, "MODULATE"}, {1, "DECAL"}, {2, "HIGHLIGHT"}, {3, "HIGHLIGHT2"}}},
	{GSRegField::Format::TEX0_CPSM, {{PSMCT32 & 0xF, "PSMCT32"}, {PSMCT16 & 0xF, "PSMCT16"}, {PSMCT16S & 0xF, "PSMCT16S"}}},
	{GSRegField::Format::TEX1_LCM, {{0, "FORMULA"}, {1, "K"}}},
	{GSRegField::Format::TEX1_MMAG, {{0, "NEAREST"}, {1, "LINEAR"}}},
	{GSRegField::Format::TEX1_MMIN, {{0, "NEAREST"}, {1, "LINEAR"}, {2, "NEAREST_MIPMAP_NEAREST"}, {3, "NEAREST_MIPMAP_LINEAR"}, {4, "LINEAR_MIPMAP_NEAREST"}, {5, "LINEAR_MIPMAP_LINEAR"}}},
	{GSRegField::Format::TEX1_MTBA, {{0, "MANUAL"}, {1, "AUTO"}}},
	{GSRegField::Format::TRXDIR_XDIR, {{0, "H->L"}, {1, "L->H"}, {2, "L->L"}, {3, "DEACTIVATE"}}},
	{GSRegField::Format::TRXPOS_DIR, {{0, "UL->LR"}, {1, "LL->UR"}, {2, "UR->LL"}, {3, "LR->UL"}}},
	{GSRegField::Format::ZBUF_PSM, {{PSMZ32 & 0xF, "PSMZ32"}, {PSMZ24 & 0xF, "PSMZ24"}, {PSMZ16 & 0xF, "PSMZ16"}, {PSMZ16S & 0xF, "PSMZ16S"}}},
	{GSRegField::Format::PACKED_XYZ2_ADC, {{0, "XYZ2"}, {1, "XYZ3"}}},
	{GSRegField::Format::PACKED_AD_ADDR, {}},
};

std::map<u32, GSRegInfo> gsPackedRegInfo;
std::map<u32, GSRegInfo> gsRegInfo;
std::map<std::string, u32> gsRegNameToId;
std::map<std::string, u32> gsPackedRegNameToId;

static void GSRegInfoInit()
{
	static bool init = false;
	if (init)
		return;
	init = true;

	auto decompressAll = [](const std::vector<std::tuple<CompressedRegIdList, CompressedFieldList>>& src,
							 std::map<u32, GSRegInfo>* dst,
							 std::map<std::string, u32>* name_to_id) {
		for (const auto& [regIdNamesCompr, fieldsCompr] : src)
		{
			std::map<GSRegField::ID, GSRegField> fields;
			for (const auto& [id, name, start_bit, end_bit, format] : fieldsCompr)
			{
				fields.insert({id, {id, name, start_bit, end_bit, format}});
			}
			for (const auto& [id, name] : regIdNamesCompr)
			{
				dst->insert({id, {id, name, fields}});
				name_to_id->insert({name, id});
			}
		}
	};

	// initialize gsRegInfo and gsRegNameToId
	decompressAll(gsRegInfoCompressed, &gsRegInfo, &gsRegNameToId);
	decompressAll(gsPackedRegInfoCompressed, &gsPackedRegInfo, &gsPackedRegNameToId);
}

static inline u64 DoShift(u64 x, u64 shift)
{
	if (shift >= 64)
		return 0;
	else
		return x << shift;
}

bool GSRegDecodeFields(u32 id, const u8* data, std::map<std::string, u64>* field_vals)
{
	GSRegInfoInit();
	const u64* data64 = (const u64*)data;
	field_vals->clear();
	const GSRegInfo& info = gsRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		const u64 mask = DoShift((u64)1, field.end_bit - field.start_bit) - 1;
		u64 bits = (data64[0] >> field.start_bit) & mask;
		field_vals->insert({field.name, bits});
	}
	return true;
}

bool GSPackedRegDecodeFields(u32 id, const u8* data, std::map<std::string, u64>* field_vals)
{
	GSRegInfoInit();
	field_vals->clear();
	const u64* data64 = (const u64*)data;
	const GSRegInfo& info = gsPackedRegInfo.at(id);
	for (const auto& [field_name, field] : info.fields)
	{
		const u64 mask = DoShift((u64)1, field.end_bit - field.start_bit) - 1;
		u64 bits = 0;
		if (field.start_bit < 64)
		{
			if (field.end_bit > 64)
			{
				// This is an error, none of the fields should cross the 64-bit boundary
				Console.Error("GSPackedRegDecodeFields(): Field %s crosses 64-bit boundary", field.name);
				return false;
			}
			bits = (data64[0] >> field.start_bit) & mask;
		}
		else
		{
			bits = (data64[1] >> (field.start_bit - 64)) & mask;
		}
		field_vals->insert({field.name, bits});
	}
	return true;
}

bool GSRegEncodeFields(u32 id, const std::map<GSRegField::ID, u64>& field_vals, u8* data)
{
	GSRegInfoInit();
	u64* data64 = (u64*)data;
	*data64 = 0;
	const GSRegInfo& info = gsRegInfo.at(id);
	for (const auto& [id, field] : info.fields)
	{
		auto it = field_vals.find(id);
		if (it != field_vals.end())
		{
			const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
			*data64 |= (it->second & mask) << field.start_bit;
		}
	}
	return true;
}

bool GSPackedRegEndcodeFields(u32 id, const std::map<GSRegField::ID, u64>& field_vals, u8* data)
{
	GSRegInfoInit();
	u64* data64 = (u64*)data;
	data64[0] = 0;
	data64[1] = 0;
	const GSRegInfo& info = gsPackedRegInfo.at(id);
	for (const auto& [id, field] : info.fields)
	{
		auto it = field_vals.find(id);
		if (it != field_vals.end())
		{
			const u64 mask = (1ULL << (field.end_bit - field.start_bit)) - 1;
			u64 bits = it->second & mask;
			if (field.start_bit < 64)
			{
				if (field.end_bit > 64)
				{
					// This is an error, none of the fields should cross the 64-bit boundary
					Console.Error("GSPackedRegEncodeFields(): Field %s crosses 64-bit boundary", field.name);
					return false;
				}
				data64[0] |= bits << field.start_bit;
			}
			else
			{
				data64[1] |= bits << (field.start_bit - 64);
			}
		}
	}
	return true;
}

std::string GSRegGetName(u32 id)
{
	GSRegInfoInit();
	return gsRegInfo.at(id).name;
}

std::string GSPackedRegGetName(u32 id)
{
	GSRegInfoInit();
	return gsPackedRegInfo.at(id).name;
}

std::string GSRegFormatField(GSRegField::ID format, u64 data)
{
	GSRegInfoInit();
	union {
		u64 u64_data;
		u32 u32_data[2];
		float f32_data[2];
	} data2;
	data2.u64_data = data;
	switch (format)
	{
		// Common formats
		case GSRegField::Format::FrameAddress: // Word address / 2048
			return StringUtil::StdStringFromFormat("0x%x", data); 
		case GSRegField::Format::Address: // Word address / 64
			return StringUtil::StdStringFromFormat("0x%x", data);
		case GSRegField::Format::BufferWidth: // Pixels / 64
			return StringUtil::StdStringFromFormat("%d", data);
		case GSRegField::Format::Unsigned: // Unsigned integer
			return StringUtil::StdStringFromFormat("%d", data);
		case GSRegField::Format::Unsigned_4: // 4 bits fractional
			return StringUtil::StdStringFromFormat("%d", data / 16.0f);
		case GSRegField::Format::Signed: // 1 bit sign
			return StringUtil::StdStringFromFormat("%d", data);
		case GSRegField::Format::Float: // 32 bits floating point
			return StringUtil::StdStringFromFormat("%f", data2.f32_data[0]);
		case GSRegField::Format::Color:
			return StringUtil::StdStringFromFormat("0x%x", data);
		case GSRegField::Format::TexelOffset: // Offset / 16
			return StringUtil::StdStringFromFormat("%d", data);
		case GSRegField::Format::Signed_2_1:
			return StringUtil::StdStringFromFormat("%f", data / 2.0f);
		case GSRegField::Format::Signed_7_4:
			return StringUtil::StdStringFromFormat("%f", data / 16.0f);
		case GSRegField::Format::TEX0_Log2Size:
			return StringUtil::StdStringFromFormat("%d", data);
		case  GSRegField::Format::ZBUF_ZMSK:
			return StringUtil::StdStringFromFormat("%x", data);
		case GSRegField::Format::PSM:
		case GSRegField::Format::ALPHA_ABD:
		case GSRegField::Format::ALPHA_C:
		case GSRegField::Format::CLAMP_WM:
		case GSRegField::Format::COLCLAMP_CLAMP:
		case GSRegField::Format::FBA_FBA:
		case GSRegField::Format::PRIM_PRIM:
		case GSRegField::Format::PRIM_IIP:
		case GSRegField::Format::PRIM_FST:
		case GSRegField::Format::PRMODECONT_AC:
		case GSRegField::Format::SCANMSK_MSK:
		case GSRegField::Format::TEST_ATST:
		case GSRegField::Format::TEST_AFAIL:
		case GSRegField::Format::TEST_DATM:
		case GSRegField::Format::TEST_ZTST:
		case GSRegField::Format::TEX0_TCC:
		case GSRegField::Format::TEX0_TFX:
		case GSRegField::Format::TEX0_CPSM:
		case GSRegField::Format::TEX1_LCM:
		case GSRegField::Format::TEX1_MMAG:
		case GSRegField::Format::TEX1_MMIN:
		case GSRegField::Format::TEX1_MTBA:
		case GSRegField::Format::TRXDIR_XDIR:
		case GSRegField::Format::TRXPOS_DIR:
		case GSRegField::Format::ZBUF_PSM:
		case GSRegField::Format::PACKED_XYZ2_ADC:
			return std::string(gsRegFieldValues.at(format).at(data));
		case GSRegField::Format::PACKED_AD_ADDR:
			return GSRegGetName(data);
		default:
			return StringUtil::StdStringFromFormat("%lld", data);
	}
}

std::string GSRegFormatField(u32 reg_id, u32const u8* data)
{

}
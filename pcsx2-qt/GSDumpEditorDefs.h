#pragma once

#include "QTUtils.h"

#include "GS/GSRegs.h"
#include "GS/GSDump.h"
#include "GS/GSLzma.h"
#include "GS/GSDrawingContext.h"
#include "GS/GSDrawingEnvironment.h"

#include "common/Console.h"
#include "common/BitUtils.h"
#include "common/Path.h"
#include "common/StringUtil.h"

#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <memory>

// Copied from GS/Renderers/Common/GSVertex.h
struct GSVertex
{
	union
	{
		struct
		{
			GIFRegST ST; // S:0, T:4
			GIFRegRGBAQ RGBAQ; // RGBA:8, Q:12
			GIFRegXYZ XYZ; // XY:16, Z:20
			union
			{
				u32 UV;
				struct
				{
					u16 U, V;
				};
			}; // UV:24
			u32 FOG; // FOG:28
		};
	};
};


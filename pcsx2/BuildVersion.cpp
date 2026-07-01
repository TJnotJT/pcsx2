// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "svnrev.h"

namespace BuildVersion
{
	const char* GitTag = GIT_TAG;
	bool GitTaggedCommit = 0;
	int GitTagHi = 0;
	int GitTagMid = 0;
	int GitTagLo = 0;
	const char* GitRev = 0;
	const char* GitHash = GIT_HASH;
	const char* GitDate = GIT_DATE;
} // namespace BuildVersion

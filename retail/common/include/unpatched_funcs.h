#ifndef UNPATCHED_FUNCS_ARM9_H
#define UNPATCHED_FUNCS_ARM9_H

#include <nds/ndstypes.h>

typedef struct unpatchedFunctions {
	u32* compressedFlagOffset;
	u32* iCompressedFlagOffset;
	u32 compressed_static_end;
	u32 ltd_compressed_static_end;
	u32* mpuDataOffset;
	u32* mpuDataOffsetAlt;
	u32* mpuDataOffset2;
	u32 mpuInitRegionOldData;
	u32 mpuInitRegionOldDataAlt;
	u32 mpuInitRegionOldData2;
	int mpuAccessOffset;
	u32 mpuOldInstrAccess;
	u32 mpuOldDataAccess;
} __attribute__ ((__packed__)) unpatchedFunctions;

#endif // UNPATCHED_FUNCS_ARM9_H

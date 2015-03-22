// Horizontal Device Metrics Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

HmtxTable::HmtxTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	nHMetrics = length >> 2;	// correct value is in HheaTable !
}

void HmtxTable::getHMetrics( int glyphNo, int* advWidth, int* lsdBear)
{
	if( glyphNo >= nHMetrics) {
		seekAbsolute( (nHMetrics - 1) << 2);
		*advWidth = readUShort();
		seekRelative( (glyphNo - (nHMetrics - 1)) << 1);
	} else {
		seekAbsolute( glyphNo << 2);
		*advWidth = readUShort();
	}
	*lsdBear = readSShort();
	dprintf3( "hmtx( %d) = { %d, %d}\n", glyphNo, *advWidth, *lsdBear);
}


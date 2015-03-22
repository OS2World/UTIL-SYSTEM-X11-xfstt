// Grid-fitting And Scan-conversion Parameters
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

GaspTable::GaspTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*int version = */readUShort();
	nRanges = readUShort();
}

int GaspTable::getFlags( int mppem)
{
	seekAbsolute( 4);

	int flags = 0;

	for( int i = nRanges; --i >= 0;) {
		int rangeMax = readUShort();
		flags = readUShort();
		if( rangeMax >= mppem)
			break;
	}

	dprintf2( "gasp::getFlags( mppem = %d) = 0x%02X)\n", mppem, flags);

	return flags;
}


// Horizontal Device Metrics
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

HdmxTable::HdmxTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*int version = */readUShort();
	nRecords = readSShort();
	recordLen = readSInt() - 1;
	assert( nRecords >= 0 && recordLen > 0);
	assert( length+8 >= nRecords * recordLen);
}

int HdmxTable::getMaxWidth( int mppemx)
{
	dprintf1( "hdmx( mppemx = %d) ", mppemx);

	seekAbsolute( 8);
	//### is it possible to avoid the linear search?
	int i;
	for( i = nRecords; --i >= 0; seekRelative( recordLen)) {
		U8 ppem = readUByte();
		if( ppem >= mppemx) {
			int maxWidth = readUByte();
			if( ppem > mppemx)
				dprintf0( "<");
			dprintf1( "= %d\n", maxWidth);
			return maxWidth;
		}
	}

	dprintf0( "not found!\n");
	return 0;
}

int HdmxTable::getGlyphWidth( int mppemx, int glyphNo)
{
	dprintf2( "hdmx( mppemx = %d, glyphNo = %d)", mppemx, glyphNo);

	seekAbsolute( 8);
	//### is it possible to avoid the linear search?
	int i;
	for( i = nRecords; --i >= 0; seekRelative( recordLen)) {
		int ppem = readUByte();
		if( ppem == mppemx) break;
	}
	if( i < 0) {
		dprintf0( " not found!\n");
		return 0;
	}

	seekRelative( glyphNo + 1);
	int width = readUByte();
	dprintf1( " = %d\n", width);
	return width;
}


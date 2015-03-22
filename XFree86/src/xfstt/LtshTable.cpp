// Linear Thresholds for advance width calculation
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

LtshTable::LtshTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*version = */readUShort();
	numGlyphs = readUShort();
}

int LtshTable::getLinearThreshold( int glyphNo)
{
	if( glyphNo < 0 || glyphNo >= numGlyphs)
		return 0;

	seekAbsolute( 4 + glyphNo);

	int yPel = readUByte();
	dprintf2( "ltsh::getThreshold( glyphNo = %d) = %d\n", glyphNo, yPel);
	return yPel; 
}


// Character Location Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

LocaTable::LocaTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
}

int LocaTable::getGlyphOffset( int glyphNo)
{
	int ofs;
	if( glyphNo < 0 || glyphNo >= maxGlyph)
		ofs = -1;
	else if( !isShort) {
		seekAbsolute( glyphNo << 2);
		ofs = readUInt();
		if( (unsigned)ofs == readUInt())
			ofs = -1;
	} else {
		seekAbsolute( glyphNo << 1);
		U16 ofs16 = readUShort();
		if( ofs16 != readUShort())
			ofs = ofs16 << 1;
		else
			ofs = -1;
	}

	return ofs;
}


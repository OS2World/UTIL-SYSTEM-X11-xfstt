// Kerning Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

KernTable::KernTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length),
	kernOffset( 0)
{
	/*version	=*/ readUShort();
	S16 nTables	= readSShort();

	for(; --nTables >= 0; seekRelative( kernLength - 14)) {
		/*subVersion	=*/ readUShort();
		kernLength	= readUShort();
		coverage	= readUShort();

		nPairs		= readUShort();
		/*searchRange	= */ readUShort();
		/*entrySelector	=*/  readUShort();
		/*rangeShift	=*/  readUShort();

		if( (coverage & FORMAT) == 0) {
			kernOffset	= tell();
			break;
		}
	}
}


int KernTable::getKerning( int /*leftChar*/, int /*rightChar*/)
{
	//### TODO
	return 0;
}


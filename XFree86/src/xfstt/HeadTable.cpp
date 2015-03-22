// Head Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

HeadTable::HeadTable( RandomAccessFile& f, int offset, int length):
	RandomAccessFile( f, offset, length)
{
	/*version	=*/ readUInt();
	/*revision	=*/ readUInt();
	checksumAdj	= readUInt();		// checkSumAdjustment
	headMagic	= readUInt();
	if( headMagic != VALIDHEAD_MAGIC)
		return;
	flags		= readUShort();
	emUnits		= readUShort();
#ifdef HAS64BIT_TYPES
	/* ctime 	=*/ readULong();	// seconds since 1904!!!
	/* mtime	=*/ readULong();	// seconds since 1904!!!
#else
	seekRelative( 16);
#endif
	xmin		= readSShort();
	ymin		= readSShort();
	xmax		= readSShort();
	ymax		= readSShort();
	macStyle	= readUShort();		// macStyle 1:bold | 2:italic
	lowestPP	= readUShort();		// lowestRecPPEM
	/* direction	=*/ readUShort();	// directionHint 1: left2right
	locaMode	= readUShort();		// 0: shortGlyph, 1: longGlyph
	/* glyfFormat	=*/ readUShort();	// glyphDataFormat == 0
}


// Hhead Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

HheaTable::HheaTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*version		=*/ readUInt();
	yAscent			= readSShort();
	yDescent		= readSShort();
	/*yGap			=*/ readSShort();	// == 0
	advWidth		= readSShort();
	minLeftBear		= readSShort();
	minRightBear		= readSShort();
	maxExtent		= readSShort();
	caretSlopeNum		= readUShort();
	caretSlopeDenom		= readUShort();
	/*reserved0		=*/ readUShort();	// == 0
	/*reserved1		=*/ readUShort();	// == 0
	/*reserved2		=*/ readUShort();	// == 0
	/*reserved3		=*/ readUShort();	// == 0
	/*reserved4		=*/ readUShort();	// == 0
	isMetric		= readUShort();
	nLongHMetrics		= readUShort();
}


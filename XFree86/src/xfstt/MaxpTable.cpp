// Maximum Parameter Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

MaxpTable::MaxpTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	reset();

	/*version	=*/ readUInt();
	numGlyphs	= readSShort();
	maxPoints	= readSShort();
	maxContours	= readSShort();

	maxCompPoints		= readUShort();
	maxCompContours		= readUShort();
	maxZones		= readUShort();
	maxTwilightPoints	= readUShort();
	maxStorage		= readUShort();
	maxFunctionDefs		= readUShort();
	maxInstructionDefs	= readUShort();
	maxStackSize		= readUShort();
	maxCodeSize		= readUShort();
	maxComponentElements	= readUShort();
	maxComponentDepth	= readUShort();
}


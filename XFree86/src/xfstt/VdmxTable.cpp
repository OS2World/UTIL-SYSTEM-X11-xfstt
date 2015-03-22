// Vertical Device Metrics
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

VdmxTable::VdmxTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*int version = */readUShort();
	nRecords = readSShort();
	nRatios = readUShort();
}

int VdmxTable::getYmax( int pelHeight, int xres, int yres, int* ymax, int* ymin)
{
	U16 offset = 0;

	for( int i = nRatios; --i >= 0;) {
		/*U8 charSet	=*/ readUByte();
		U8 xRatio	= readUByte();
		U8 yStartRatio	= readUByte();
		U8 yEndRatio	= readUByte();
		U16 tmp		= readUShort();
		if( (yres * xRatio >= xres * yStartRatio)
		 && (yres * xRatio <= xres * yEndRatio)) {
			offset	= tmp;
			break;
		}
	}

	if( offset == 0)
		return 0;

	seekAbsolute( offset);
	int nrecs	= readUShort();
	U8 startSize	= readUByte();
	U8 endSize	= readUByte();
	if( pelHeight < startSize || pelHeight > endSize)
		return 0;

	//### should be a binary search
	while( --nrecs >= 0) {
		U16 ph	= readUShort();
		S16 y1	= readSShort();
		S16 y2	= readSShort();
		if( pelHeight == ph) {
			*ymax = y1;
			*ymin = y2;
			return 1;
		}
	}

	return 1;
}


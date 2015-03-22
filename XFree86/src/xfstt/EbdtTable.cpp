// Embedded Bitmap Data
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"
#include <string.h>

EbdtTable::EbdtTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
	/*int version = */readUInt();	// == 0x00020000
}

int EbdtTable::readBitmap( int format, U8* bitmap, GlyphMetrics* gm)
{
	int height, width;
	int hBearX, hBearY;
	int vBearX, vBearY;
	int hAdv, vAdv;

	// get glyph metric
	switch( format) {
	case 1:
	case 2:
	case 8:	// small metrics
		height	= readUByte();
		width	= readUByte();
		hBearX	= readSByte();
		hBearY	= readSByte();
		hAdv	= readUByte();
		break;
	case 3:	// obsolete
	case 4: // unsupported
	default:
		return -1;
	case 5:	// metrics in EBLC instead
		break;
	case 6:
	case 7:
	case 9:	// big metrics
		height	= readUByte();
		width	= readUByte();
		hBearX	= readSByte();
		hBearY	= readSByte();
		hAdv	= readUByte();
		vBearX	= readSByte();
		vBearY	= readSByte();
		vAdv	= readUByte();
		break;
	}

	// get glyph bitmap
	switch( format) {
	case 1:
	case 6:	// byte aligned bitmap
		//memcpy( bitmap, tellAbs(), height * ((width + 7) >> 3));
		{
			int len = height * ((width + 7) >> 3);
			for( U8* p = bitmap; --len >= 0; ++p)
				*p = readUByte();
		}
		break;
	case 2:
	case 5:
	case 7:	// bit aligned bitmap
		{
		for( int rem = 0, h = height; --h >= 0;) {
			int data = 0, w;
			for( w = width; w >= 8; w -= 8) {
				int i = readUByte();
				data |= i >> rem;
				*(bitmap++) = data;
				data = i << (8 - rem);
			}
			rem = w & 7;
			if( rem)
				*(bitmap++) = data & (~0 << (8 - rem));
		}
		break;
		}
	case 8:
	case 9:	// composite bitmap
		break;
	}

	return 0;
}


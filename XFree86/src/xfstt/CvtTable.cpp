// Control Value Table
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

CvtTable::CvtTable( RandomAccessFile& f, int offset, int length):
	RandomAccessFile( f, offset, length)
{
	nVals = length >> 1;
}


// Font Program
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

FpgmTable::FpgmTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
}


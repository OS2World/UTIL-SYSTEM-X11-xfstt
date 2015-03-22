// Preparation Program
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"

PrepTable::PrepTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length)
{
}


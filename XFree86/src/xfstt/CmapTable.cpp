// Character Map Table
// (C) Copyright 1997-1998 Herbert Duerr
// mac7 style ttf support by Ethan Fischer

#include "ttf.h"

CmapTable::CmapTable( RandomAccessFile& f, int offset, int length)
:	RandomAccessFile( f, offset, length),
	format(-1), subtableOffset( 0)
{
	/*version	= */readUShort();
	S16 nSubTables	= readSShort();

	for( int i = nSubTables; --i >= 0;) {
		/*platformID	= */readUShort();
		/*encodingID	= */readUShort();
		subtableOffset	= readUInt();
		//###
	}

	if (subtableOffset) {
		seekAbsolute(subtableOffset);
		format = readUShort();
	}

	switch (format) {
	case BYTE_ENCODING: // normal Mac format
		//format	= readUShort();
		//length	= readUShort();
		//version	= readUShort();
		break;
	case HIGH_BYTE_MAPPING: // not supported
		break;
	case SEGMENT_MAPPING: // normal Windows format
		//format	= readUShort();	// == 4
		/*length	= */readUShort();
		/*version	= */readUShort();
		f4NSegments	= readSShort() >> 1;
		//searchRange	= readUShort();
		//entrySelector	= readUShort();
		//rangeShift	= readUShort();
		//short[] endCount;
		//readShort(); // reserved
		//short[] startCount;
		//short[] idDelta;
		//short[] idRangeOffset;
		break;
	case TRIMMED_MAPPING: // newer Mac fonts use this?
		//format		= readUShort();
		/*length		= */readUShort();
		/*version		= */readUShort();
		f6FirstCode		= readUShort();
		f6EntryCount	= readUShort();
		break;
	case -1: // no encoding tables
		break;
	default: // unknown table format
		break;
	}
}

int CmapTable::char2glyphNo( char char8)
{
	return unicode2glyphNo( char8);
}

int CmapTable::unicode2glyphNo( U16 unicode)
{
	if( format == -1)
		return 0;
	else if( format == BYTE_ENCODING) {
		if( unicode > 255)
			return 0;
		seekAbsolute( subtableOffset + 6 + unicode);
		int glyphNo = readUByte();
		dprintf2( "MAC.cmap[ %d] = %d\n", unicode, glyphNo);
		return glyphNo;
	} else if ( format == TRIMMED_MAPPING) {
		if( (unicode < f6FirstCode) || (unicode >= f6FirstCode + f6EntryCount))
			return 0;
		seekAbsolute( subtableOffset + 10 + (unicode - f6FirstCode) * 2);
		int glyphNo = readUShort();
		return glyphNo;
	}

	// search for endCount
	int lower = 0;
	int upper = f4NSegments - 1;
	int index = 0;
	while( upper > lower) {
		index = (upper + lower) >> 1;
		//### read header and do more checking
		seekAbsolute( subtableOffset + 14 + (index << 1));
		if( readUShort() >= unicode)
			upper = index;
		else
			lower = index + 1;
	}

	// check corresponding startCount
	int ofs = subtableOffset + 16 + (f4NSegments << 1) + (lower << 1);
	seekAbsolute( ofs);
	U16 startCount = readUShort();
		if( unicode < startCount)
			return 0;

	// get idDelta
	seekAbsolute( ofs += (f4NSegments << 1));
	int idDelta = readSShort();

	// get idRangeOffset
	seekAbsolute( ofs += (f4NSegments << 1));
	U16 idRangeOffset = readUShort();

	// calculate GlyphIndex
	if( idRangeOffset == 0)
		return (unicode + idDelta);

	seekAbsolute( ofs + idRangeOffset + ((unicode - startCount) << 1));
	return readUShort();
}

U16 CmapTable::nextUnicode( U16 unicode)
{
	++unicode;

	if( format == -1)
		return 0;
	else if( format == BYTE_ENCODING) {
		if( unicode > 255)
			return 0;
		return unicode;
	} else if ( format == TRIMMED_MAPPING) {
		if( unicode < f6FirstCode)
			return f6FirstCode;
		else if( unicode >= f6FirstCode + f6EntryCount)
			return 0;
		
		return unicode;
	}

	int lower = 0;
	int upper = f4NSegments - 1;
	if( lower > upper)
	    return 0;
	while( upper > lower) {
		int index = (upper + lower) >> 1;
		seekAbsolute( subtableOffset + 14 + (index << 1));
		if( readUShort() >= unicode)
			upper = index;
		else
			lower = index + 1;
	}

	// check corresponding startCount
	seekAbsolute( subtableOffset + 16 + (f4NSegments << 1) + (lower << 1));
	int startCount = readUShort();
	if( startCount == 0xffff)
	    return 0;
	if( startCount > unicode)
		return startCount;
	return unicode;
}

U16 CmapTable::firstUnicode()
{
	if( format == -1)
		return 0;
	else if( format == BYTE_ENCODING)
		return 0;
	else if ( format == TRIMMED_MAPPING)
		return f6FirstCode;

	seekAbsolute( subtableOffset + 16 + (f4NSegments << 1));
	U16 i = readUShort();
	dprintf1( "First Unicode = %d\n", i);
	return i;
}

U16 CmapTable::lastUnicode()
{
	if( format == -1)
		return 0;
	else if( format == BYTE_ENCODING)
		return 255;
	else if ( format == TRIMMED_MAPPING)
		return f6FirstCode + f6EntryCount - 1;

	seekAbsolute( subtableOffset + 14 + ((f4NSegments-2) << 1));
	U16 i = readUShort();
	dprintf1( "Last Unicode = %d\n", i);
	return i;
}


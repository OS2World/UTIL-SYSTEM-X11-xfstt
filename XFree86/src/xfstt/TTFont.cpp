// General handling of *ttf files
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"
#include <stdio.h>
#include <string.h>

TTFont::TTFont( char* fileName, int infoOnly):
	RandomAccessFile( fileName),
	nameTable( 0),	headTable( 0),	maxpTable( 0),
	cmapTable( 0),	locaTable( 0),	glyphTable( 0),
	fpgmTable( 0),	prepTable( 0),	cvtTable( 0),
	hheaTable( 0),	hmtxTable( 0),	
	os2Table( 0),
	ltshTable( 0),	hdmxTable( 0),	vdmxTable( 0),
	gaspTable( 0),	kernTable( 0),
	//postTable( 0),
	ebdtTable( 0), eblcTable( 0),
	mortTable(0), vheaTable( 0)
{
	dprintf1( "TTFont( \"%s\");\n", fileName);

	if( openError())
		return;

	/*U32 version		= */readUInt();
	short nTables		= readSShort();
	/*U16 searchRange	= */readUShort();
	/*U16 entrySelector	= */readUShort();
	/*U16 rangeShift	= */readUShort();

	for( int i = nTables; --i >= 0;) {
		int name	= readUInt();
		/*int checksum	=*/ readUInt();
		U32 offset	= readUInt();
		U32 length	= readUInt();
		if( length == 0)
			continue;
		if( offset >= getLength() || offset + length > getLength()) {
			headTable = 0;
			return;
		}
		if( infoOnly
		&& name!=NAME_MAGIC && name!=OS2_MAGIC && name!=HEAD_MAGIC)
			continue;
		switch( name) {
		case NAME_MAGIC:
			nameTable = new NameTable( *this, offset, length);
			break;
		case HEAD_MAGIC:
			headTable = new HeadTable( *this, offset, length);
			break;
		case MAXP_MAGIC:
			maxpTable = new MaxpTable( *this, offset, length);
			break;
		case CMAP_MAGIC:
			cmapTable = new CmapTable( *this, offset, length);
			break;
		case LOCA_MAGIC:
			locaTable = new LocaTable( *this, offset, length);
			break;
		case GLYF_MAGIC:
			glyphTable = new GlyphTable( *this, offset, length);
			break;
		case LTSH_MAGIC:
			ltshTable = new LtshTable( *this, offset, length);
			break;
		case OS2_MAGIC:
			os2Table = new OS2Table( *this, offset, length);
			break;
		case VDMX_MAGIC:
			vdmxTable = new VdmxTable( *this, offset, length);
			break;
		case CVT_MAGIC:
			cvtTable = new CvtTable( *this, offset, length);
			break;
		case FPGM_MAGIC:
			fpgmTable = new FpgmTable( *this, offset, length);
			break;
		case PREP_MAGIC:
			prepTable = new PrepTable( *this, offset, length);
			break;
		case HDMX_MAGIC:
			hdmxTable = new HdmxTable( *this, offset, length);
			break;
		case HHEA_MAGIC:
			hheaTable = new HheaTable( *this, offset, length);
			break;
		case HMTX_MAGIC:
			hmtxTable = new HmtxTable( *this, offset, length);
			break;
		case KERN_MAGIC:
			kernTable = new KernTable( *this, offset, length);
			break;
		case POST_MAGIC:
			//postTable = new PostTable( *this, offset, length);
			break;
		case GASP_MAGIC:
			gaspTable = new GaspTable( *this, offset, length);
			break;
		case EBDT_MAGIC:
		case BDAT_MAGIC:
			//###ebdtTable = new EbdtTable( *this, offset, length);
			break;
		case EBLC_MAGIC:
		case BLOC_MAGIC:
			//###eblcTable = new EblcTable( *this, offset, length);
			break;
		case VHEA_MAGIC:
			//vheaTable = new VheaTable( *this, offset, length);
			break;
		case MORT_MAGIC:
			//mortTable = new MortTable( *this, offset, length);
			break;
		default:
			// System.out.println( "???");
			break;
		}
	}

	if( !(nameTable && headTable)) {
		if( headTable) delete headTable;
		headTable = 0;
	}

	if( infoOnly)
		return;

	if( !(maxpTable && cmapTable && locaTable && glyphTable)) {
		if( headTable) delete headTable;
		headTable = 0;
	}

	if( headTable == 0) {
		dprintf0( "Incomplete TT file\n");
		return;
	}

	locaTable->setupLoca( headTable->shortLoca(), maxpTable->numGlyphs);
	hmtxTable->setupHmtx( hheaTable->nLongHMetrics);
}


TTFont::~TTFont()
{
	closeRAFile();

	if( nameTable)	delete nameTable;
	if( headTable)	delete headTable;
	if( maxpTable)	delete maxpTable;
	if( cmapTable)	delete cmapTable;
	if( locaTable)	delete locaTable;
	if( glyphTable)	delete glyphTable;

	if( os2Table)	delete os2Table;
	if( cvtTable)	delete cvtTable;
	if( fpgmTable)	delete fpgmTable;
	if( prepTable)	delete prepTable;

	if( hheaTable)	delete hheaTable;
	if( hmtxTable)	delete hmtxTable;

	if( ltshTable)	delete ltshTable;
	if( hdmxTable)	delete hdmxTable;
	if( vdmxTable)	delete vdmxTable;

	if( gaspTable)	delete gaspTable;
	if( kernTable)	delete kernTable;
}

int TTFont::badFont()
{
	return (openError() || !headTable || headTable->badHeadMagic());
} 

int TTFont::getEmUnits()
{
	return headTable->emUnits;
}

void TTFont::getFontInfo( FontInfo* fi)
{
	if (os2Table) {
		fi->firstChar	= os2Table->firstCharNo;
		fi->lastChar	= os2Table->lastCharNo;

		for( int i = 0; i < 10; ++i)
			fi->panose[ i] = os2Table->panose[ i];
	} else {
		fi->firstChar	= 0x0020;	// space
		fi->lastChar	= 0x007f;	// end of Latin 1 in Unicode

		for( int i = 0; i < 10; ++i)
			fi->panose[ i] = 0;		// any
	}

	// we need an ascii name even if we only have unicode
	// => use a conversion buffer
	char conv[ 256];
	char* faceName = nameTable->getString( 1, 4, &fi->faceLength, conv);
	if( !faceName) {
		faceName = "Unknown";
		fi->faceLength = strlen( faceName);
	}
	if( fi->faceLength > 32)
		fi->faceLength = 32;
	strncpy( fi->faceName, faceName, fi->faceLength);
	if( fi->faceLength < 31)
		fi->faceName[ fi->faceLength] = 0;
}


int TTFont::getGlyphNo8( char char8)
{
	return cmapTable->char2glyphNo( char8);
}

int TTFont::getGlyphNo16( int unicode)
{
	return cmapTable->unicode2glyphNo( unicode);
}

#if 0	// currently not used
int TTFont::doCharNo8( GlyphTable* g, int char8)
{
	return doGlyphNo( g, getGlyphNo8( char8));
}

int TTFont::doCharNo16( GlyphTable* g, int unicode)
{
	return doGlyphNo( g, getGlyphNo16( unicode));
}
#endif

int TTFont::getMaxWidth( int mppemx)
{
	int maxWidth = 0;
	if( hdmxTable)
		maxWidth = hdmxTable->getMaxWidth( mppemx);
	if( maxWidth == 0) {
		maxWidth = headTable->xmax - headTable->xmin;
		maxWidth += headTable->emUnits >> 5;	// +3%
		maxWidth = maxWidth * mppemx / headTable->emUnits;
		dprintf1( "using maxWidth %d instead\n", maxWidth);
	}
	return maxWidth;
}

int TTFont::getGlyphWidth( int mppemx, int glyphNo)
{
	int width =  0;
	if( hdmxTable)
		width = hdmxTable->getGlyphWidth( mppemx, glyphNo);
	if( width == 0) {
		int dummy;
		hmtxTable->getHMetrics( glyphNo, &width, &dummy);
		//###if( width == 0)
		//###	width = getMaxWidth( mppemx):
		width += headTable->emUnits >> 5;	// +3%
		width = width * mppemx / headTable->emUnits;
		dprintf1( "using width %d instead\n", width);
	}
	return width;
}


// verify on reference implementation

int TTFont::patchGlyphCode( GlyphTable* g, int glyphNo)
{
	return 0;
}

int TTFont::checksum( U8* buf, int len)
{
	len = (len + 3) >> 2;
	int sum = 0;
	for( U8* p = buf; --len >= 0; p += 4) {
		int val = (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3];
		sum += val;
	}
	return sum;
}

void TTFont::updateChecksums()
{
	U8* buf = base;

	int nTables = (buf[4] << 8) + buf[5];
	//printf( "nTables = %d\n", nTables);
	U8* headTable = 0;
	for( int i = 0; i < nTables; ++i) {
		U8* b = &buf[ 12 + i * 16];
		int name = (b[0]<<24) + (b[1]<<16) + (b[2]<<8) + b[3];
		int offset = (b[8]<<24) + (b[9]<<16) + (b[10]<<8) + b[11];
		int length = (b[12]<<24) + (b[13]<<16) + (b[14]<<8) + b[15];
		//printf( "offset = %08X, length = %08X\n", offset, length);
		int check = checksum( buf + offset, length);
		b[4] = (U8) (check >> 24);
		b[5] = (U8) (check >> 16);
		b[6] = (U8) (check >> 8);
		b[7] = (U8) check;
		//printf( "checksum[%d] = %08X\n", i, check);
		if( name == 0x68656164) {
			headTable = buf + offset;
			//printf( "headOffset = %08X\n", offset);
		}
	}

	int check = checksum( buf, getLength()) - 0xB1B0AFBA;
	//printf( "csAdjust = %08X\n", check);
	headTable[8] = (U8) (check >> 24);
	headTable[9] = (U8) (check >> 16);
	headTable[10] = (U8) (check >> 8);
	headTable[11] = (U8) check;
}

int TTFont::write2File( char* filename)
{
	FILE* fd = fopen( filename, "wb");
	if( fd) {
		int len = fwrite( base, 1, getLength(), fd);
		fclose( fd);
		return len;
	}
	return -1;
}


#include <string.h>
#include <ctype.h>

// result has to be preset with the category name "-category-",
// returns "-category-family-weight-slant-setwidth-TT-"
//###	"pixelsize-pointsize-xres-yres-spacing-avgwidth-charset-encoding"

int TTFont::getXLFDbase( char* result)
{
//#define XLFDEXT "-normal-tt-0-0-0-0-p-0-iso8859-1"
//#define XLFDEXT	"-normal-tt-"

	// some fonts have only unicode names -> try to convert them to ascii
	char convbuf[ 256];
	int lenFamily;
	char* strFamily = nameTable->getString( 1, 1, &lenFamily, convbuf);
	if( !strFamily) {
		strFamily = "Unknown";
		lenFamily = strlen( strFamily);
	}

	int lenSub;
	char* strSubFamily = nameTable->getString( 1, 2, &lenSub, convbuf);
	if( !strFamily) {
		strSubFamily = "tt";
		lenSub = strlen( strSubFamily);
	}

	char* p = result + strlen( result);
	p[0] = '-';
	strncpy( ++p, strFamily, lenFamily);
	for( p[ lenFamily] = 0; *p; ++p)
		if( *p == '-')
			*p = ' ';
	if (os2Table) {
		strcpy( p, (os2Table->selection & 32) ? "-bold" : "-medium");
		strcat( p, (os2Table->selection & 1) ? "-i" : "-r");
	} else {
		strcpy( p, (headTable->macStyle & 1) ? "-bold" : "-medium");
		strcat( p, (headTable->macStyle & 2) ? "-i" : "-r");
	}

	strcat( p, "-normal-");
	p += strlen( p);
	strncpy( p, strSubFamily, lenSub);
	for( p[ lenSub] = 0; *p; ++p)
		if( *p == '-')
			*p = ' ';
	*(p++) = '-';
	*p = 0;

	for( p = result; *p; ++p)
		*p = tolower( *p);

	dprintf1( "xlfd = \"%s\"\n", result);
	return strlen( result);
}


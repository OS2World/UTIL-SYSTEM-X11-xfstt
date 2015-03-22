// Header for the xfstt (X Font Server for TT Files) font engine
// (C) Copyright 1997-1998 Herbert Duerr

// usage example to get font info, glyph extents and glyph bitmaps:
//
//      raster = new Rasterizer();
//      font = new TTFont( "fontname.ttf");
//      raster->getFontInfo( &fontInfo);
//      raster->setPixelSize( 20, 0, 0, 20);
//      raster->getFontExtent( &fontExtent);
//      delete font;
//      delete raster;

#include "arch.h"

#include <stdio.h>
#include <assert.h>

#ifndef MULDIV
        #define MULDIV(a,b,c) (int)(((S64)(a) * (b) + (c>>1)) / (c))
#endif

#ifndef DEBUG
        #define dprintf0( s)
        #define dprintf1( s, x1)
        #define dprintf2( s, x1, x2)
        #define dprintf3( s, x1, x2, x3)
        #define STATIC static
#else
        #define dprintf0( s)                    fprintf( outfile, (s))
        #define dprintf1( s, x1)                fprintf( outfile, (s), (x1))
        #define dprintf2( s, x1, x2)            \
                fprintf( outfile, (s), (x1), (x2))
        #define dprintf3( s, x1, x2, x3)        \
                fprintf( outfile, (s), (x1), (x2), (x3))
        #define STATIC
#endif

#ifndef MEMDEBUG
        inline void cleanupMem() {}
#else
        void cleanupMem();
#endif

#ifndef MAGNIFY
        extern int MAGNIFY;
#endif

class RandomAccessFile
{
protected:
        U8 *ptr, *base;         // low offset for frequently used members
private:
        U8* absbase;            //### hack for fileOffset();
        int length;
public:
        RandomAccessFile( char* fileName);
        RandomAccessFile( RandomAccessFile& f, int offset, int _length) {
                length = _length;
                absbase = f.base;
                ptr = base = f.base + offset;
        }
//###   virtual ~RandomAccessFile()     {}

        int openError()                 { return (absbase==0);}
        void closeRAFile();

        void reset()                    { ptr = base;}
        void seekAbsolute( int ofs)     { ptr = base + ofs;}
        void seekRelative( int rel)     { ptr += rel;}
        int tell()                      { return (ptr - base);}
        int fileOffset()                { return (ptr - absbase);}
        U32 getLength()                 { return length;}

        U32 calcChecksum();

        // these inlined functions are generic and should be optimized
        // for specific processors (alignment, endianess, ...)

        S8 readSByte() {
                S8 i = ptr[0];
                ptr += 1;
                return i;
        }
        U8 readUByte() {
                U8 i = ptr[0];
                ptr += 1;
                return i;
        }
        S16 readSShort() {
                S16 i = ptr[0];
                i = (i<<8) | ptr[1];
                ptr += 2;
                return i;
        }
        U16 readUShort() {
                U16 i = ptr[0];
                i = (i<<8) | ptr[1];
                ptr += 2;
                return i;
        }
        S32 readSInt() {
                S32 i = ptr[0];
                i = (i<<8) | ptr[1];
                i = (i<<8) | ptr[2];
                i = (i<<8) | ptr[3];
                ptr += 4;
                return i;
        }
        U32 readUInt() {
                U32 i = ptr[0];
                i = (i<<8) | ptr[1];
                i = (i<<8) | ptr[2];
                i = (i<<8) | ptr[3];
                ptr += 4;
                return i;
        }

#ifdef HAS64BIT_TYPES
        S64 readSLong() {
                S64 i = (S64)readSInt() << 32;
                return i | readUInt();
        }
        U64 readULong() {
                U64 i = (U64)readUInt() << 32;
                return i | readUInt();
        }
#endif /* HAS64BIT_TYPES */

        void writeByte( U8 byte) {
                *(ptr++) = byte;
        }
};

void* allocMem( int size);              // mmaping allocation
void* shrinkMem( void* ptr, int oldsize, int newsize);
void deallocMem( void* ptr, int size);


class NameTable;
class HeadTable;
class MaxpTable;
class CmapTable;
class LocaTable;
class GlyphTable;
class Rasterizer;

class FpgmTable;
class PrepTable;

class HheaTable;
class HmtxTable;
class OS2Table;
class CvtTable;

class LtshTable;
class HdmxTable;
class VdmxTable;

class GaspTable;

class EbdtTable;
class EblcTable;
class EbscTable;

//class PclTable;
class KernTable;
//class PostTable;
class VheaTable;
class MortTable;

enum{ ON_CURVE = 1, X_TOUCHED = 0x40, Y_TOUCHED = 0x80, END_SUBGLYPH = 0x100};
typedef struct{ int xnow, ynow, xold, yold, flags
#ifdef WIN32
        , xgoal, ygoal, fgoal
#endif
;} point;

struct FontInfo
{
        U16     firstChar, lastChar;
        U8      panose[10];
        int     faceLength;
        char    faceName[ 32];
};

// table of tables
class TTFont
: public RandomAccessFile
{
friend Rasterizer;
public: //### perftest needs maxpTable
        NameTable* nameTable;
        HeadTable* headTable;
        MaxpTable* maxpTable;
        CmapTable* cmapTable;
        LocaTable* locaTable;
        GlyphTable* glyphTable;

        FpgmTable* fpgmTable;
        PrepTable* prepTable;
        CvtTable* cvtTable;

        HheaTable* hheaTable;
        HmtxTable* hmtxTable;
        OS2Table* os2Table;

        LtshTable* ltshTable;
        HdmxTable* hdmxTable;
        VdmxTable* vdmxTable;

        GaspTable* gaspTable;
        KernTable* kernTable;

        EbdtTable* ebdtTable;
        EblcTable* eblcTable;
        MortTable* mortTable;
        VheaTable* vheaTable;

        enum {
                NAME_MAGIC      = 0x6E616D65,
                HEAD_MAGIC      = 0x68656164,
                MAXP_MAGIC      = 0x6D617870,
                CMAP_MAGIC      = 0x636D6170,
                LOCA_MAGIC      = 0x6C6F6361,
                GLYF_MAGIC      = 0x676C7966,
                LTSH_MAGIC      = 0x4C545348,
                HDMX_MAGIC      = 0x68646D78,
                VDMX_MAGIC      = 0x56444D58,
                OS2_MAGIC       = 0x4F532F32,
                FPGM_MAGIC      = 0x6670676D,
                PREP_MAGIC      = 0x70726570,
                CVT_MAGIC       = 0x63767420,
                HHEA_MAGIC      = 0x68686561,
                VHEA_MAGIC      = 0x76686570,
                HMTX_MAGIC      = 0x686D7478,
                KERN_MAGIC      = 0x6B65726E,
                POST_MAGIC      = 0x706F7374,
                GASP_MAGIC      = 0x67617370,
                EBLC_MAGIC      = 0x45424C43,
                BLOC_MAGIC      = 0x626C6F63,
                EBDT_MAGIC      = 0x45424454,
                BDAT_MAGIC      = 0x62646174,
                MORT_MAGIC      = 0x6D6F7274
        };

        int* endPoints;
        point* points;
public:
        TTFont( char* fileName, int infoOnly = 0);
        ~TTFont();

        int badFont();
        void getFontInfo( FontInfo* fi);

        int getGlyphNo8( char charNo);
        int getGlyphNo16( int charNo);
        int getEmUnits();

        int getMaxWidth( int mppemx);
        int getGlyphWidth( int mppemx, int glyphNo);
        int getGlyphMetrics( int glyphNo);

        int getXLFDbase( char* xlfd);

        // for comparing with reference implementation
        int patchGlyphCode( GlyphTable* glyph, int instruction);
        int checksum( U8* buf, int len);
        void updateChecksums();
        int write2File( char* filename);
        void patchName( U8* patchData, int patchLength);
};


// name table
class NameTable
: public RandomAccessFile
{
        int strBase;
        int nRecords;
public:
        NameTable( RandomAccessFile& f, int offset, int length);

        enum {  COPYRIGHT=0,    FAMILY=1,       SUBFAMILY=2,    UNIQUE=3,
                FULLNAME=4,     VERSION=5,      POSTSCRIPT=6,   TRADEMARK=7};

        char* getString( int platformId, int strId, int* len, char* conv=0);
};


// font specific flags
class HeadTable
: public RandomAccessFile
{
        U32 headMagic;
public: //###
        U32 checksumAdj;
        U16 flags;
        U16 emUnits;
        S16 xmin, ymin;
        S16 xmax, ymax;
        U16 macStyle;
        U16 lowestPP;
        U16 locaMode;

        enum{ FONTCRC_MAGIC = 0xB1B0AFBA, VALIDHEAD_MAGIC = 0x5F0F3CF5};

        enum{ Y_BASELINE=1, X_BASELINE=2, CODE_VARIES=4,
                FORCE_INT=8, WIDTH_VARIES=16};

        enum{ BOLD=1, ITALIC=2};

public:
        HeadTable( RandomAccessFile& f, int offset, int length);

        int shortLoca() { return locaMode == 0;}
        int badHeadMagic() { return headMagic != VALIDHEAD_MAGIC;}
};


// font limits
class MaxpTable
: public RandomAccessFile
{
friend TTFont;
friend Rasterizer;

        U16 numGlyphs;
        U16 maxPoints;
        U16 maxContours;

        U16 maxCompPoints;
        U16 maxCompContours;
        U16 maxZones;
        U16 maxTwilightPoints;
        U16 maxStorage;
        U16 maxFunctionDefs;
        U16 maxInstructionDefs;
        U16 maxStackSize;
        U16 maxCodeSize;
        U16 maxComponentElements;
        U16 maxComponentDepth;

public:
        MaxpTable( RandomAccessFile& f, int offset, int length);

        U16 getNumGlyphs() { return numGlyphs;}
};


// character code to glyph number mapping
class CmapTable
: public RandomAccessFile
{
        enum { BYTE_ENCODING = 0, HIGH_BYTE_MAPPING = 2,
                SEGMENT_MAPPING = 4, TRIMMED_MAPPING = 6 }; // format
        int format;
        int subtableOffset;
        int f4NSegments;
        int f6FirstCode;
        int f6EntryCount;
public:
        CmapTable( RandomAccessFile& f, int offset, int length);

        int char2glyphNo( char char8);
        int unicode2glyphNo( U16 unicode);

        U16 nextUnicode( U16 unicode);
        U16 firstUnicode();
        U16 lastUnicode();
};


// glyph number to glyph data offset mapping
class LocaTable
: public RandomAccessFile
{
        int maxGlyph;
        int isShort;
public:
        LocaTable( RandomAccessFile& f, int offset, int length);

        void setupLoca( int _isShort, int _maxGlyph) {
                isShort = _isShort;
                maxGlyph = _maxGlyph;
        }

        int getGlyphOffset( int glyphNo);
};


// glyph data
class GlyphTable
: /*public*/ RandomAccessFile
{
friend Rasterizer;
        S16 xmin;
        //S16 xmin, ymin;
        //S16 xmax, ymax;

        int codeOffset;
        int codeLength;

public: //### needed by verify.cpp
        enum{ ON_CURVE = 0x01, X_SHORT = 0x02, Y_SHORT = 0x04,
                F_SAME = 0x08, X_EXT = 0x10, Y_EXT = 0x20};

        int nEndPoints;
        int* endPoints;
        int nPoints;
        point* points;

public:
        GlyphTable( RandomAccessFile& f, int offset, int length);

        void setupGlyph( point* _points, int* _endPoints) {
                endPoints = _endPoints;
                points = _points;
        }

        int getGlyphData( int glyphOffset, LocaTable* locaTable, Rasterizer*);

private:
        int getCompositeGlyphData( int gn, LocaTable* locaTable, Rasterizer*);

        enum{   ARGS_ARE_WORDS  = 0x0001,
                ARGS_ARE_XY     = 0x0002,
                ROUND_XY_TO_GRID= 0x0004,
                HAS_A_SCALE     = 0x0008,
                // reserved     = 0x0010,
                // NO_OVERLAP   = 0x0010, which one is the right one?
                MORE_COMPONENTS = 0x0020,
                HAS_XY_SCALE    = 0x0040,
                HAS_2X2_SCALE   = 0x0080,
                HAS_CODE        = 0x0100,
                USE_MY_METRICS  = 0x0200};
};


// Hhea
class HheaTable
: public RandomAccessFile
{
friend TTFont;
friend Rasterizer;
        int yAscent;
        int yDescent;
        int advWidth;
        int minLeftBear;
        int minRightBear;
        int maxExtent;
        int caretSlopeNum;
        int caretSlopeDenom;
        int isMetric;
        int nLongHMetrics;

        HheaTable( RandomAccessFile& f, int offset, int length);
};


// Hmtx
class HmtxTable
: public RandomAccessFile
{
        int nHMetrics;
public:
        HmtxTable( RandomAccessFile& f, int offset, int length);

        void setupHmtx( int h) { nHMetrics = h;}

        void getHMetrics( int glyphNo, int* advWidth, int* lsdBear);
};


// OS/2
class OS2Table
: public RandomAccessFile
{
public:
        S16     weightClass;
        U16     avg_width;
        U8      panose[10];
        U32     unicodeRange[ 4];
        U16     firstCharNo;
        U16     lastCharNo;
        U16     selection;
        U16     typoAscent;
        U16     typoDescent;
        U16     typoGap;
        U16     winAscent;
        U16     winDescent;

        enum UnicodeRangeFlags {
                LATIN_0 = 0,    LATIN_1 = 1,    LATIN_A = 2,    LATIN_B = 3,
                IPA_EXT = 4,    SPACE_MOD = 5,  DIAC_MARKS = 6, GREEK_0 = 7,
                GREEK_SYM = 8,  CYRILLIC = 9,   ARMENIAN = 10,  HEBREW_0 = 11,
                HEBREW_AB = 12, ARABIC_0 = 13,  ARABIC_1 = 14,  DEVANAGARI = 15,
                BENGALI = 16,   GURMUKHI = 17,  GUJARATI = 18,  ORIYA = 19,
                TAMIL = 20,     TELUGU = 21,    KANNADA = 22,   MALAYALAM = 23,
                THAI = 24,      LAO = 25,       GEORGIAN_0 = 26,GEORGIAN_1 = 27,
                HANGUL_1 = 28,  LATIN_EA = 29,  GREEK_A = 30,   PUNCT_0 = 31,
                SUBSUPER = 32,  CURRENCY = 33,  DIAC_SYM = 34,  LETTER = 35,
                NUMFORMS = 36,  ARROWS = 37,    MATHOP = 38,    MISCTECH = 39,
                CNTRLPIC = 40,  OCR = 41,       ENCALPHA = 42,  BOXDRAW = 43,
                BOXELEM = 44,   GSHAPES = 45,   MISCSYMS = 46,  DINGBATS = 47,
                CJK_SYMS = 48,  HIRAGANA = 49,  KATAGANA = 50,  BOPOMOFO = 51,
                HANGUL_2 = 52,  CJK_MISC = 53,  CJK_ENC = 54,   CJK_COMP = 55,
                HANGUL_0 = 56,  RESERVED1 = 57, RESERVED2 = 58, CJK_IDEO0 = 59,
                PRIVATE = 60,   CJK_IDEO1 = 61, ALF_PRES = 62,  ARB_PRES0 = 63,
                HALFMARK = 64,  CJK_FORM = 65,  SFORMV = 66,    ARB_PRESB = 67,
                WIDTHFORM = 68, SPECIALS = 69
        };

        enum SelectionFlags {
                ITALIC = 1,     UNDERSCORE = 2, NEGATIVE = 4,   OUTLINED = 8,
                STRIKEOUT = 16, BOLD = 32,      REGULAR = 64
        };
public:
        OS2Table( RandomAccessFile& f, int offset, int length);
};


// LTSH
class LtshTable
: public RandomAccessFile
{
        int numGlyphs;
public:
        LtshTable( RandomAccessFile& f, int offset, int length);

        int getLinearThreshold( int glyphNo);
};


// hdmx
class HdmxTable
: public RandomAccessFile
{
        int nRecords;
        int recordLen;
public:
        HdmxTable( RandomAccessFile& f, int offset, int length);

        int getMaxWidth( int mppemx);
        int getGlyphWidth( int mppemx, int glyphNo);
};


// VDMX
class VdmxTable
: public RandomAccessFile
{
        int nRecords;
        int nRatios;
public:
        VdmxTable( RandomAccessFile& f, int offset, int length);
        int getYmax( int pelHeight, int xres, int yres, int* ymax, int* ymin);
};


// cvt
class CvtTable
: public RandomAccessFile
{
        int nVals;
public:
        CvtTable( RandomAccessFile& f, int offset, int length);

        int numVals() { return nVals;}
        int nextVal() { return readSShort();}
};


// fpgm
class FpgmTable
: public RandomAccessFile
{
friend TTFont;
        FpgmTable( RandomAccessFile& f, int offset, int length);
};


// prep
class PrepTable
: public RandomAccessFile
{
friend TTFont;
        PrepTable( RandomAccessFile& f, int offset, int length);
};


// gasp
class GaspTable
: public RandomAccessFile
{
        int nRanges;
public:
        GaspTable( RandomAccessFile& f, int offset, int length);

        enum{ GASP_GRIDFIT = 1, GASP_DOGRAY = 2};
        int getFlags( int mppem);
};



// Kern kerning table
class KernTable
: public RandomAccessFile
{
        int     kernOffset;
        U16     nPairs;
        U16     kernLength;
        U16     coverage;
public:
        KernTable( RandomAccessFile& f, int offset, int length);

        enum {HORIZONTAL=1, MINIMUM=2, CROSS=4, OVERRIDE=8, FORMAT=0xff00};

        int getKerning( int leftChar, int rightChar);
};


struct FontExtent
{
        int xBlackboxMin, xBlackboxMax;
        int yBlackboxMin, yBlackboxMax;
        int xLeftMin, xLeftMax;
        int xRightMin, xRightMax;
        int yAscentMin, yAscentMax;
        int yDescentMin, yDescentMax;
        int xAdvanceMin, xAdvanceMax;
        int yAdvanceMin, yAdvanceMax;
        int yWinAscent, yWinDescent;

        U8* buffer;     // hack
        U8* bitmaps;    // hack
        int buflen;     // hack
        int bmplen;     // hack
        int numGlyphs;  // hack
        int bmpFormat;  // hack
        // format of buffer:
        //      CharInfo[ numGlyphs]
        //      bitmaps[ numGlyphs]
};

struct GlyphMetrics
{
        int xBlackbox, yBlackbox;
        int xOrigin, yOrigin;
        int xAdvance, yAdvance;
};

struct CharInfo
{
        GlyphMetrics gm;
        int offset;
        int length;
        int tmpofs;     // scratchpad offset
};


// EBLC embedded bitmap locations
class EblcTable
: public RandomAccessFile
{
        enum { HORIZONTAL = 0x01, VERTICAL = 0x02};
public:
        EblcTable( RandomAccessFile& f, int offset, int length);

        void readStrike( int glyphNo, int _ppemx, int _ppemy);
        void readSubTableArray( int glyphNo, int ofsSTA);
        void readSubTable( int first, int last);
};

// EBDT embedded bitmap data
class EbdtTable
: public RandomAccessFile
{
public:
        EbdtTable( RandomAccessFile& f, int offset, int length);

        int readBitmap( int format, U8* bitmap, GlyphMetrics* gm);
};

// EBSC embedded bitmap scaling info
class EbscTable
: public RandomAccessFile
{
public:
        EbscTable( RandomAccessFile& f, int offset, int length);
};


class GraphicsState {
friend Rasterizer;      // only Rasterizer needs this

        GraphicsState() {}

        void init( point** p);
        int absNewMeasure( int dx, int dy);
        int absOldMeasure( int dx, int dy);
        void movePoint( point& pp, int len10D6);
        void recalc();

        int f_vec_x;            // freedom vector
        int p_vec_x;            // projection vector
        int dp_vec_x;           // dual projection vector

        int f_vec_y;
        int p_vec_y;
        int dp_vec_y;

        int flags;
        int move_x;
        int move_y;

        point *zp0, *zp1, *zp2;
        int rp0, rp1, rp2;      // reference points
        int loop;               // loop counter
        int auto_flip;
        int cvt_cut_in;
        int round_state;
        int round_phase;
        int round_period;
        int round_thold;
        int min_distance;
        int swidth_cut_in;
        int swidth_value;
        int delta_base;
        int delta_shift;
        int instr_control;
        int dropout_control;
};

enum{ UNITY2D14 = 0x4000};
enum{ SHIFT = 6, SUBS = 64};
enum {VGARES = 96};

// scan line converter
class Rasterizer
{
private:
        typedef struct{ RandomAccessFile* f; int offset; int length;} FDefs;
        typedef FDefs IDefs;

        // often accessed members should be here (low offsets)

        int *stack, *stackbase;         // stack grows upward
        point* p[2];                    // points (twilight + action zone)
        int* endPoints;
        int nPoints[2];                 // number of points in zone 0 and 1
        int nEndPoints;

        int* cvt;               // control value table
        int* stor;              // storage area
        FDefs* fdefs;           // function definitions
        IDefs* idefs;           // instruction definitions

        TTFont* ttFont;

        // misc

        enum { UNDERLINE = 1, STRIKEOUT = 2, SUBSCRIPT = 4, SUPERSCRIPT = 8};
        int flags;

        enum {  INVALID_FONT, NOT_READY,
                TRAFO_APPLIED, FONT_DONE} status;

        // memory allocation status

        int sizePoints[2], sizeContours, sizeStack;
        int sizeCvt, sizeStor, sizeFDefs, sizeIDefs;

        // used by interpreter

        int xx, xy, yx, yy, xxexp;
        int pointSize;
        int mppem, mppemx, mppemy;

        GraphicsState gs, default_gs;

        // resulting bitmap
        #define SCANLINEPAD     (1<<LOGSLP)
        #define SLPMASK         (SCANLINEPAD-1)

        #if (LOGSLP == 3)
                #define TYPESLP U8
        #elif (LOGSLP == 4)
                #define TYPESLP U16
        #elif (LOGSLP == 5)
                #define TYPESLP U32
        #elif (LOGSLP == 6)
                #define TYPESLP U64
        #else
                #error "illegal value for LOGSLP"
        #endif

public: //### clean up showttf so this can become private
        int format;     // >0 scanlinepad, <0 maxwidth

        int width, height;
        unsigned int length;
        int dX;

        int grid_fitting;       // ==0: none, <0: enabled, >0: enabled+working
        int anti_aliasing;      // ==0: none, <0: enabled, >0: enabled+working

public:
        Rasterizer( int _grid_fitting=1, int _anti_aliasing=0,
                int _sizeTwilight=0, int _sizePoints=0, int _sizeContours=0,
                int _sizeStack=0, int _sizeCvt=0, int _sizeStor=0,
                int _sizeFDefs=0);
        ~Rasterizer();

        void useTTFont( TTFont* _ttFont, int _flags=0);
        void setPixelSize( int xx,int xy,int yx,int yy);
        void setPointSize( int xx,int xy,int yx,int yy, int xres,int yres);
        void getFontExtent( FontExtent* fe);

        int putChar8Bitmap( char c8, U8* bmp, U8* bmpend, GlyphMetrics* gm);
        int putChar16Bitmap( int c16, U8* bmp, U8* bmpend, GlyphMetrics* gm);
        int putGlyphBitmap( int glyph, U8* bmp, U8* bmpend, GlyphMetrics* gm);

        void printOutline( void);

private:
friend GlyphTable;
        void applyTransformation();
        void scaleGlyph();
        int scaleX( int x, int y) { return (((xx*x+xy*y) + 16) >> 5) << xxexp;}
        int scaleY( int y, int x) { return (((yx*x+yy*y) + 16) >> 5) << xxexp;}
        void hintGlyph( GlyphTable* g, int offset, int length);
        void putGlyphData( int ne, int np, int* ep, point* pp, int gn, int xm);

        void initInterpreter();
        void endInterpreter();
        void calcCVT();

        void execHints( RandomAccessFile* const f, int offset, int length);
        void execOpcode( RandomAccessFile* const f);
        static void skipHints( RandomAccessFile* const f);

        int round( int x) const;

        void interpolate( point&pp, const point& pRef1, const point& pRef2);
        STATIC void doIUP0( point* const first, point* const last);
        STATIC void doIUP1( point* const first, point* const last);
        STATIC void iup0( point* const pp,
                const point* const pRef1, const point* const pRef2);
        STATIC void iup1( point* const pp,
                const point* const pRef1, const point* const pRef2);
        int newMeasure( const point& p2, const point& p1);
        int oldMeasure( const point& p2, const point& p1);
        static void newLine2vector( const point& p1, const point& p2,
                int& vx, int& vy);
        static void oldLine2vector( const point& p1, const point& p2,
                int& vx, int& vy);

        static void openDraw();
        static void closeDraw();
        void drawGlyph( U8* const startbmp, U8* const endbmp);
        void drawBitmap( U8* const bmp, int height, int dX);
        static void drawContour( point* const first, point* const last);
        static const point* drawPoly( const point& p0, const point& p1,
                const point& p2);
        static void drawSegment( int x1,int y1, int x2,int y2);
        static void bezier1( int x0,int y0, int x1,int y1, int x2,int y2);
        void antiAliasing2( U8* bmp);
};


// hint opcodes

enum {
        // pushing values on the stack

        NPUSHB          = 0x40, // push n bytes
        NPUSHW          = 0x41, // push n words
        PUSHB00         = 0xB0, // push bytes
        PUSHB01, PUSHB02, PUSHB03,
        PUSHB04, PUSHB05, PUSHB06, PUSHB07,
        PUSHW00         = 0xB8, // push words
        PUSHW01, PUSHW02, PUSHW03,
        PUSHW04, PUSHW05, PUSHW06, PUSHW07,

        // accessing the storage area

        RS              = 0x43, // read store
        WS              = 0x42, // write store

        // accessing the CVT table

        WCVTP           = 0x44, // write cvt in pixel units
        WCVTF           = 0x70, // write cvt in Funits
        RCVT            = 0x45, // read cvt element

        // accessing the graphics state

        SVTCA0          = 0x00, // set (freedom and projection) vector to yaxis
        SVTCA1          = 0x01, // set (freedom and projection) vector to xaxis
        SPVTCA0         = 0x02, // set projection vector to yaxis
        SPVTCA1         = 0x03, // set projection vector to xaxis
        SFVTCA0         = 0x04, // set freedom vector to yaxis
        SFVTCA1         = 0x05, // set freedom vector to xaxis
        SPVTL0          = 0x06, // set projection vector parallel to line
        SPVTL1          = 0x07, // set projection vector perpendicular to line
        SFVTL0          = 0x08, // set freedom vector parallel to line
        SFVTL1          = 0x09, // set freedom vector perpendicular to line
        SFVTPV          = 0x0E, // set freedom to projection vector
        SDPVTL0         = 0x86, // set dual projection vector parallel to line
        SDPVTL1         = 0x87, // set dual projection vector perpendicular to line
        SPVFS           = 0x0A, // set projection vector from stack
        SFVFS           = 0x0B, // set freedom vector from stack
        GPV             = 0x0C, // get projection vector
        GFV             = 0x0D, // get freedom vector
        SRP0            = 0x10, // set reference point rp0
        SRP1, SRP2,
        SZP0            = 0x13, // set zone pointer zp0
        SZP1, SZP2,
        SZPS            = 0x16, // set zp0, zp1, zp2
        RTHG            = 0x19, // round to half grid
        RTG             = 0x18, // round to grid
        RTDG            = 0x3D, // round to double grid
        RDTG            = 0x7D, // round down to grid
        RUTG            = 0x7C, // round up to grid
        ROFF            = 0x7A, // round off
        SROUND          = 0x76, // super round
        S45ROUND        = 0x77, // super round 45 degrees
        SLOOP           = 0x17, // set loop counter
        SMD             = 0x1A, // set minimum distance
        INSTCTRL        = 0x8E, // intruction execution control

        // scan conversion control

        SCANCTRL        = 0x85, // scan conversion control
        SCANTYPE        = 0x8D, //
        SCVTCI          = 0x1D, // set cvt cut_in
        SSWCI           = 0x1E, // set single width cut_in
        SSW             = 0x1F, // set single width value
        FLIPON          = 0x4D, // sets auto_flip
        FLIPOFF         = 0x4E, // reset auto_flip
        SANGW           = 0x7E, // set angle weight
        SDB             = 0x5E, // set delta base
        SDS             = 0x5F, // set delta shift

        // measurements

        GC0             = 0x46, // get current coordinate projected
        GC1             = 0x47, // get original coordinate projected
        SCFS            = 0x48, // set coordinates from stack
        MD0             = 0x49, // measure distance in current outline
        MD1             = 0x4A, // measure distance in original outline
        MPPEM           = 0x4B, // measure pixels per em in p_vector direction
        MPS             = 0x4C, // measure point size

        // outline manipulation

        FLIPPT          = 0x80, // flip point
        FLIPRGON        = 0x81, // flip range on
        FLIPRGOFF       = 0x82, // flip range off
        SHP0            = 0x32, // shift point by last point rp2 zp1
        SHP1            = 0x33, // shift point by last point rp1 zp0
        SHC0            = 0x34, // shift contour by point
        SHC1            = 0x35, // shift contour by point
        SHZ0            = 0x36, // shift zone by point
        SHZ1            = 0x37, // shift zone by point
        SHPIX           = 0x38, // shift zone by pixel
        MSIRP0          = 0x3A, // move stack indirect relative point, no set
        MSIRP1          = 0x3B, // move stack indirect relative point, set rp0
        MDAP0           = 0x2E, // move direct absolute point, no rounding
        MDAP1           = 0x2F, // move direct absolute point, rounding
        MIAP0           = 0x3E, // move indirect absolute point, no rounding
        MIAP1           = 0x3F, // move indirect absolute point, rounding
        MDRP00          = 0xC0, // move direct relative point
        MDRP01, MDRP02, MDRP03,
        MDRP04, MDRP05, MDRP06, MDRP07,
        MDRP08, MDRP09, MDRP0A, MDRP0B,
        MDRP0C, MDRP0D, MDRP0E, MDRP0F,
        MDRP10, MDRP11, MDRP12, MDRP13,
        MDRP14, MDRP15, MDRP16, MDRP17,
        MDRP18, MDRP19, MDRP1A, MDRP1B,
        MDRP1C, MDRP1D, MDRP1E, MDRP1F,
        MIRP00          = 0xE0, // move indirect relative point
        MIRP01, MIRP02, MIRP03,
        MIRP04, MIRP05, MIRP06, MIRP07,
        MIRP08, MIRP09, MIRP0A, MIRP0B,
        MIRP0C, MIRP0D, MIRP0E, MIRP0F,
        MIRP10, MIRP11, MIRP12, MIRP13,
        MIRP14, MIRP15, MIRP16, MIRP17,
        MIRP18, MIRP19, MIRP1A, MIRP1B,
        MIRP1C, MIRP1D, MIRP1E, MIRP1F,
        ALIGNRP         = 0x3C, // align rp
        ALIGNPTS        = 0x27, // align points
        ISECT           = 0x0F, // move point to intersection of two lines
        AA              = 0x7F, // adjust angle (no longer supported)
        IP              = 0x39, // interpolate point
        UTP             = 0x29, // untouch point
        IUP0            = 0x30, // interpolate untouched points, x direction
        IUP1            = 0x31, // interpolate untouched points, y direction
        DELTAP1         = 0x5D, // delta exception p1
        DELTAP2         = 0x71, // delta exception p2
        DELTAP3         = 0x72, // delta exception p3
        DELTAC1         = 0x73, // delta exception c1
        DELTAC2         = 0x74, // delta exception c2
        DELTAC3         = 0x75, // delta exception c3

        // stack manipulation

        DUP             = 0x20, // duplicate stack top
        POP             = 0x21, // pop
        CLEAR           = 0x22, // clear entire stack
        SWAP            = 0x23, // swap
        DEPTH           = 0x24, // return depth of stack
        CINDEX          = 0x25, // copy from stack index
        MINDEX          = 0x26, // move from stack index
        ROLL            = 0x8A, // roll top three stack elements

        // control flow

        IF              = 0x58, // if
        ELSE            = 0x1B, // else
        EIF             = 0x59, // endif
        JROT            = 0x78, // jump relative on true
        JMPR            = 0x1C, // jump relative unconditionally
        JROF            = 0x79, // jump relative on false

        // arithmetic

        LT              = 0x50, // less than
        LTEQ            = 0x51, // less than or equal
        GT              = 0x52, // greater than
        GTEQ            = 0x53, // greater than or equal
        EQ              = 0x54, // equal
        NEQ             = 0x55, // not equal
        ODD             = 0x56, // odd
        EVEN            = 0x57, // even
        AND             = 0x5A, // logical and
        OR              = 0x5B, // logical or
        NOT             = 0x5C, // logical not
        ADD             = 0x60, // add
        SUB             = 0x61, // subtract
        DIV             = 0x62, // divide
        MUL             = 0x63, // multiply
        ABS             = 0x64, // absolute
        NEG             = 0x65, // negative
        FLOOR           = 0x66, // floor
        CEILING         = 0x67, // ceiling
        MAX             = 0x8B, // maximum
        MIN             = 0x8C, // minimum
        ROUND00         = 0x68, // round engine independant
        ROUND01, ROUND02, ROUND03,
        NROUND00        = 0x6C, // round engine dependant
        NROUND01, NROUND02, NROUND03,

        // subroutines

        FDEF            = 0x2C, // function definition
        ENDF            = 0x2D, // end of function
        CALL            = 0x2B, // call
        LOOPCALL        = 0x2A, // loop and call
        IDEF            = 0x89, // instruction definition

        // miscelleneous

        DBG             = 0x4F, // debug
        GETINFO         = 0x88  // get info about glyph and scaler
};

enum {  // round state, do the constants in the spec make any sense?
        ROUND_OFF       ,       //= 5,
        ROUND_GRID      ,       //= 1,
        ROUND_DOWN      ,       //= 3,
        ROUND_UP        ,       //= 4,
        ROUND_HALF      ,       //= 0,
        ROUND_DOUBLE    ,       //= 2,
        ROUND_SUPER     ,       //= 6,
        ROUND_SUPER45           //= 7
};

enum {  // instruction control
        INHIBIT_GRID_FIT        = 1,
        IGNORE_CVT_CHANGES      = 2
};

enum {  // getinfo parameters
        SCALER_VERSION  = 1,
        GLYPH_ROTATED   = 2,
        GLYPH_STRETCHED = 4,
        // getinfo results
        WIN_SCALER      = 3,
        IS_ROTATED      = 0x100,
        IS_STRETCHED    = 0x200
};


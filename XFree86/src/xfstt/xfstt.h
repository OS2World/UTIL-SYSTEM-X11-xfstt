// Header for the X Font Server for TT Fonts
// (C) Copyright 1997-1998 Herbert Duerr

#define TTFN_VERSION 0x0102

typedef struct
{
	int	nameOfs;
	U8	nameLen;
	U8	pathLen;
	U8	xlfdLen;
	U8	pad;

	U8	charSet;
	U8	slant;

	// panose attributes
	U8	bFamilyType;
	U8	bSerifStyle;
	U8	bWeight;
	U8	bProportion;
	U8	bContrast;
	U8	bStrokeVariation;
	U8	bArmStyle;
	U8	bLetterForm;
	U8	bMidLine;
	U8	bXHeight;
} TTFNdata;

typedef struct
{
	char	magic[4];		// == TTFN
	char	type[4];		// == INFO or NAME
	U16	version;
	U16	key;
	U32	crc;
	//TTFNdata ttfn[];
} TTFNheader;

typedef struct
{
	int	numFonts;

	int	maxGlyphs;
	int	maxPoints;
	int	maxTwilight;
	int	maxContours;
	int	maxStack;
	int	maxStore;
	int	maxCvt;
	int	maxFDefs;
	int	maxIDefs;
} TTFNinfo;

typedef struct
{
	int pixel[4];		// pixelsize
	int point[4];		// pointsize
	int resolution[2];	// resolution
	int flags;
} FontParams;


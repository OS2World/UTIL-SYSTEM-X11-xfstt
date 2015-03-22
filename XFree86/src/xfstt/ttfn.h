// TTFN is an alternative to XLFD suited for arbitrarily scalable fonts
//
// why?
// a list request for the XLFD pattern "-*-Times-*100*" does not tell
// what this 100 means: Is it the pixelSize, the pointSize, the x resolution,
// the y resolution or the average width? Is it part of a 2x2 matrix used for
// rotating fonts? This *100* also matches 1000...1009 or 1100, 2100...
//
// The approach here is to be more specific depending on the context:
//	OpenFont name != ListFont result != ListFont pattern
// OpenFont names provide the fontname and rasterizer specific attributes
// ListFont results provide the fontname and raster independant attributes

/***************************************************************************
 * this struct is for XListFonts results
 ***************************************************************************/

// panose[0]	= bFamilyType
// panose[1]	= bSerifStyle
// panose[2]	= bWeight
// panose[3]	= bProportion
// panose[4]	= bContrast
// panose[5]	= bStrokeVariation
// panose[6]	= bArmStyle
// panose[7]	= bLetterForm
// panose[8]	= bMidLine
// panose[9]	= bXHeight

typedef struct {
	U8	nameLen;
	char	magic[2];			// magic == "TT"
	char	charset;			// U=unicode, A=ascii, S=symbol
	char	panoseMagic;			// 'P'
	char	panose[10][2];			// in hex
	char	modifier;			// italic
	char	underscore;
	//char	fontName[];
} TPFontName;

/***************************************************************************
 * this naming is for XOpenFont fontnames
 ***************************************************************************/

/* an TTFN openfont name consists of a start pattern, one char tags
 * with decimal values, an underscore and a font name
 *
 * TT			(start pattern)
 *
 * MnnnMnnnMnnnMnnn	(scaling matrix: xx yy xy yx)
 *			default.xx = default.pointsize
 *			default.yy = xx
 *			default.xy = 0;
 *			default.yx = -xy
 *
 * RnnnRnnn		(resolution: xres yres)
 *			default.yres = xres
 *
 * Fnnn			(flags)
 *			default.flags = 0
 *			bit0 = underlined
 *			bit1 = strikeout
 *			bit2 = subscript
 *			bit3 = superscript
 *			undefined bits must be zero
 *
 * example:
 *	12 point subscripted Arial has the font name "TTM12F4_Arial"
 */


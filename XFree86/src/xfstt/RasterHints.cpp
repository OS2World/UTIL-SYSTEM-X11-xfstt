// Hint Interpreter
// (C) Copyright 1997-1998 Herbert Duerr

#include "ttf.h"
#include <math.h>	// we need sqrt() or hypot() for normalisation

#define FSHIFT 1.0
#define MEMSLACK 2

void Rasterizer::initInterpreter()
{
	if( !grid_fitting || !ttFont)
		return;

	assert( SCANLINEPAD == sizeof(TYPESLP)<<3);
	assert( SCANLINEPAD == 1 << LOGSLP);
	assert( SCANLINEPAD == SLPMASK + 1);

	nPoints[0] = ttFont->maxpTable->maxTwilightPoints;
	if( sizePoints[0] < nPoints[0]) {
		if( sizePoints[0]) delete[] p[0];
		sizePoints[0] = MEMSLACK * nPoints[0];
		p[0] = new point[ sizePoints[0]];
	}

	point nullpoint = {0, 0, 0, 0, 0};
	for( int i = 0; i < nPoints[0]; ++i)
		p[0][i] = nullpoint;

	if( sizeStack < ttFont->maxpTable->maxStackSize) {
		if( sizeStack) delete[] stackbase;
		sizeStack = MEMSLACK * ttFont->maxpTable->maxStackSize;
		stackbase = stack = new int[ sizeStack];
	}

	if( ttFont->cvtTable && sizeCvt < ttFont->cvtTable->numVals()) {
		if( sizeCvt) delete[] cvt;
		sizeCvt	= MEMSLACK * ttFont->cvtTable->numVals();
		cvt = new int[ sizeCvt];
	}

	if( sizeStor < ttFont->maxpTable->maxStorage) {
		if( sizeStor) delete[] stor;
		sizeStor = MEMSLACK * ttFont->maxpTable->maxStorage;
		stor = new int[ sizeStor];
	}

	if( sizeFDefs < ttFont->maxpTable->maxFunctionDefs) {
		if( sizeFDefs) delete[] fdefs;
		sizeFDefs = MEMSLACK * ttFont->maxpTable->maxFunctionDefs;
		fdefs = new FDefs[ sizeFDefs];
	}

	if( 0 == sizeIDefs) {
		sizeIDefs = 256;
		idefs = new IDefs[ sizeIDefs];
	}

	for( int j = 0; j < sizeIDefs; ++j)
		idefs[ j].length = 0;

#ifdef DEBUG
	setbuffer( stdout, 0, 0);
#endif
	if( ttFont->fpgmTable) {
		gs.init( p);
		stack = stackbase;
		dprintf0( "--- fpgm start ---\n");
		execHints( ttFont->fpgmTable, 0, ttFont->fpgmTable->getLength());
		dprintf0( "--- fpgm done ---\n");
	}
}

void Rasterizer::endInterpreter()
{
	if( sizeIDefs)		delete[] idefs;
	if( sizeFDefs)		delete[] fdefs;
	if( sizeStor)		delete[] stor;
	if( sizeCvt)		delete[] cvt;
	if( sizeStack)		delete[] stackbase;
/*
	if( sizeContours)	delete[] endPoints;
	if( sizePoints[1])	delete[] p[1];
*/
	if( sizePoints[0])	delete[] p[0];
}

void Rasterizer::calcCVT()
{
	CvtTable* cvtTab = ttFont->cvtTable;
	if( cvtTab == 0)
		return;

	cvtTab->reset();
	int n = cvtTab->numVals();
	assert( n >= 0 && n <= sizeCvt);

	int scale = xx + yy;
	for( int i = 0; i < n; ++i) {
		int val = cvtTab->nextVal();
		// (ld 2048 = 11) - (SHIFT = 6) = 5
		cvt[ i] = ((val * scale + 32) >> 6) << xxexp;
		dprintf3( "cvt[%3d] = %5d  -> %5d\n", i, val, cvt[ i]);
	}

	if( ttFont->prepTable == 0)
		return;

	dprintf0( "=== starting glyph prep ===\n");
	grid_fitting = 1;
	gs.init( p);
	int preplen = ttFont->prepTable->getLength();
	stack = stackbase;
	execHints( ttFont->prepTable, 0, preplen);
	default_gs = gs;
	dprintf0( "=== glyph prep done ===\n");
}

void GraphicsState::init( point** p)
{
	flags		= X_TOUCHED;
	move_x		= UNITY2D14;
	move_y		= 0;

	p_vec_x		= UNITY2D14;
	p_vec_y		= 0;
	f_vec_x		= UNITY2D14;
	f_vec_y		= 0;
	dp_vec_x	= UNITY2D14;
	dp_vec_y	= 0;
	zp2 = zp1 = zp0	= p[1];
	rp2 = rp1 = rp0	= 1;
	loop		= 1;
	auto_flip	= 1;
	cvt_cut_in	= (17 << SHIFT) >> 4;
	round_state	= ROUND_GRID;
	round_phase	= 0;
	round_period	= (1 << SHIFT);
	round_thold	= 0;
	min_distance	= (1 << SHIFT);
	swidth_cut_in	= 0;
	swidth_value	= 0;
	delta_base	= 9;
	delta_shift	= 3;
	instr_control	= 0;
	dropout_control	= 1;
}

// some inlined utilities

/*inline*/ int Rasterizer::round( int x) const
{
	int y = (x >= 0) ? +x : -x;
	switch( gs.round_state) {
	case ROUND_OFF:
		return x;
	case ROUND_GRID:
		y += 32;
		// fall through
	case ROUND_DOWN:
		y &= -64;
		break;
	case ROUND_UP:
		y = (y | 63) + 1;
		break;
	case ROUND_HALF:
		y = (y | 63) - 31;
		break;
	case ROUND_DOUBLE:
		y = (y + 16) & -32;
		break;
	case ROUND_SUPER:
		y -= gs.round_phase - gs.round_thold;
		y &= -gs.round_period;
		y += gs.round_phase;
		dprintf2( "\tsround(%d) = %d\t", x, (x<0)?-y:+y);
		break;
	case ROUND_SUPER45:
		y -= gs.round_phase - gs.round_thold;
		y -= y % gs.round_period;
		y += gs.round_phase;
		dprintf2( "\tsround45(%d) = %d\t", x, (x<0)?-y:+y);
		break;
	}
	if( y < 0) return 0;
	return (x >= 0) ? +y : -y;
}

inline int GraphicsState::absNewMeasure( int dx11D6, int dy11D6)
{
	dprintf2( "\ndx = %d, dy = %d", dx11D6, dy11D6);
	dprintf2( ",\tpx = %d, py = %d", p_vec_x, p_vec_y);

	int dist = dx11D6 * p_vec_x + dy11D6 * p_vec_y + 0x2000;
	dist >>= 14;
	return dist;
}

inline int GraphicsState::absOldMeasure( int dx11D6, int dy11D6)
{
	dprintf2( "\ndx = %d, dy = %d", dx11D6, dy11D6);
	dprintf2( ",\tdpx = %d, dpy = %d", dp_vec_x, dp_vec_y);

	int dist = dx11D6 * dp_vec_x + dy11D6 * dp_vec_y + 0x2000;
	dist >>= 14;
	return dist;
}

inline int Rasterizer::newMeasure( const point& p2, const point& p1)
{
	int dist = gs.absNewMeasure( p2.xnow - p1.xnow, p2.ynow - p1.ynow);
	dprintf3( "\nnewMeasure p[%d]-p[%d] = %f",
		&p2-p[1], &p1-p[1], dist/FSHIFT);
	
	return dist;
}

inline int Rasterizer::oldMeasure( const point& p2, const point& p1)
{
	int dist = gs.absOldMeasure( p2.xold - p1.xold, p2.yold - p1.yold);
	dprintf3( "\noldMeasure p[%d]-p[%d] = %f",
		&p2-p[1], &p1-p[1], dist/FSHIFT);
	return dist;
}

void Rasterizer::newLine2vector( const point& p2, const point& p1,
	int& vx, int &vy)
{
	//### how can it be that df1 or df2 are bigger than 16 bits ?
	float f1 = p2.xnow - p1.xnow;
	float f2 = p2.ynow - p1.ynow;
#if 1
	float f3 = sqrt( UNITY2D14*UNITY2D14 / (f1*f1 + f2*f2));
#else
	float f3 = UNITY2D14 / hypot( f1, f2);
#endif
	vx = (int)(f1 * f3);
	vy = (int)(f2 * f3);

	dprintf2( "\t(%d %d) - ", p2.xnow, p2.ynow);
	dprintf2( "(%d %d)", p1.xnow, p1.ynow);
	dprintf2( "\nvx vy = %f %f", vx/FSHIFT, vy/FSHIFT);
}

void Rasterizer::oldLine2vector( const point& p2, const point& p1,
	int& vx, int &vy)
{
	//### how can it be that df1 or df2 are bigger than 16 bits ?
	float f1 = p2.xold - p1.xold;
	float f2 = p2.yold - p1.yold;
#if 1
	float f3 = sqrt( UNITY2D14*UNITY2D14 / (f1*f1 + f2*f2));
#else
	float f3 = UNITY2D14 / hypot( f1, f2);
#endif
	vx = (int)(f1 * f3);
	vy = (int)(f2 * f3);

	dprintf2( "\t(%d %d) - ", p2.xold, p2.yold);
	dprintf2( "(%d %d)", p1.xold, p1.yold);
	dprintf2( "\nvx vy = %f %f", vx/FSHIFT, vy/FSHIFT);
}

inline void GraphicsState::movePoint( point& pp, int len11D6)
{
	dprintf1( "\nmovePoint by %f", len11D6/FSHIFT);
	dprintf2( "\t(%d %d)", pp.xnow, pp.ynow); 

	pp.xnow += (len11D6 * move_x) >> 14;
	pp.ynow += (len11D6 * move_y) >> 14;
	pp.flags |= flags;

	dprintf2( "\t-> (%d %d)\n", pp.xnow, pp.ynow); 
}

void GraphicsState::recalc()
{
	flags = f_vec_x ? X_TOUCHED : 0;
	if( f_vec_y) flags |= Y_TOUCHED;

	int fp_cross = absNewMeasure( f_vec_x, f_vec_y);
	if( fp_cross) {
		move_x = (f_vec_x << 14) / fp_cross;
		move_y = (f_vec_y << 14) / fp_cross;
	} else
		move_x = move_y = 0;
}

// interpreter

inline void Rasterizer::execOpcode( RandomAccessFile* const f)
{
	// optimisation question: how does one convince
	// g++ to keep the "this" pointer in a register?
	// all that is needed to make hinting really fast
	// is keeping these variables in registers
	// 1) this 2) m 3) n,
	// keeping i and pp in registers would help too.
	//
	// While I'm at it: Why the heck does g++/egcs/pgcc take
	// a much bigger (up to 5K) stack frame than it really
	// needs? MS VC++ code only grabs 40 bytes!

	register int m;
	register point* pp;
	int n;

	assert( stack >= stackbase);

	int opc = f->readUByte();
	dprintf2( "\n::%05X %02X\t", f->fileOffset() - 1, opc);
	switch( opc) {

	// pushing onto the stack

	case NPUSHB:
		opc = f->readUByte() + PUSHB00 - 1;
		// fall through
	case PUSHB00: case PUSHB01:
	case PUSHB02: case PUSHB03:
	case PUSHB04: case PUSHB05:
	case PUSHB06: case PUSHB07:
		m = opc - (PUSHB00 - 1);
		dprintf1( "PUSHB * %d\n>\t\t", m);
		while( --m >= 0) {
			++stack;
			*stack = f->readUByte();
			dprintf1( "%d,\t", *stack);
			if( (m&7) == 0) dprintf0( "\n>\t\t");
		}
		break;

	case NPUSHW:
		opc = f->readUByte() + PUSHW00 - 1;
		// fall through
	case PUSHW00: case PUSHW01:
	case PUSHW02: case PUSHW03:
	case PUSHW04: case PUSHW05:
	case PUSHW06: case PUSHW07:
		m = opc - (PUSHW00 - 1);
		dprintf1( "PUSHW * %d\n>\t\t", m);
		while( --m >= 0) {
			*(++stack) = f->readSShort();
			dprintf1( "%d,\t", *stack);
			if( (m&7) == 0) dprintf0( "\n>\t\t");
		}
		break;

	// accessing the storage area

	case RS:
		m = *stack;
		assert( m >= 0 && m < sizeStor);
		*stack = stor[ m];
		dprintf2( "RS store[%d] = %d", m, *stack);
		break;
	case WS:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "WS %d -> store[%d]", m, n);
		assert( n >= 0 && n < sizeStor);
		stor[ n] = m;
		break;

	// accessing the control value table

	case WCVTP:
		m = *(stack--);
		n = *(stack--);
		assert( n >= 0 && n < sizeCvt);
		cvt[ n] = m;
		dprintf3( "WCVTP %d -> cvt[%d] = %d", m, n, cvt[n]);
		break;
	case WCVTF:
		m = *(stack--);
		n = *(stack--);
		assert( n >= 0 && n < sizeCvt);
		//### how does one scale a scalar with the ((xx,xy),(yx,yy)) matrix???
		cvt[ n] = ((m * (xx + yy) + 32) >> 6) << xxexp;
		dprintf3( "#WCVTF %d -> cvt[%d] = %d\n", m, n, cvt[n]);
		break;
	case RCVT:
		m = *stack;
		assert( m >= 0 && m < sizeCvt);
		*stack = cvt[ m];
		dprintf2( "RCVT cvt[%d] = %d", m, *stack);
		break;

	// accessing the graphics state

	case SVTCA0:
		dprintf0( "SVTCA0");
		gs.dp_vec_x = gs.p_vec_x = gs.f_vec_x = 0;
		gs.dp_vec_y = gs.p_vec_y = gs.f_vec_y = UNITY2D14;
		gs.move_x = 0;
		gs.move_y = UNITY2D14;
		gs.flags = Y_TOUCHED;
		break;
	case SVTCA1:
		dprintf0( "SVTCA1");
		gs.dp_vec_x = gs.p_vec_x = gs.f_vec_x = UNITY2D14;
		gs.dp_vec_y = gs.p_vec_y = gs.f_vec_y = 0;
		gs.move_x = UNITY2D14;
		gs.move_y = 0;
		gs.flags = X_TOUCHED;
		break;
	case SPVTCA0:
		dprintf0( "SPVTCA0");
		gs.dp_vec_x = gs.p_vec_x = 0;
		gs.dp_vec_y = gs.p_vec_y = UNITY2D14;
		if( gs.f_vec_y) {
			gs.move_x = (gs.f_vec_x << 14) / gs.f_vec_y;
			gs.move_y = (gs.f_vec_y << 14) / gs.f_vec_y;
		} else
			gs.move_x = gs.move_y = 0;
		break;
	case SPVTCA1:
		dprintf0( "SPVTCA1");
		gs.dp_vec_x = gs.p_vec_x = UNITY2D14;
		gs.dp_vec_y = gs.p_vec_y = 0;
		if( gs.f_vec_x) {
			gs.move_x = (gs.f_vec_x << 14) / gs.f_vec_x;
			gs.move_y = (gs.f_vec_y << 14) / gs.f_vec_x;
		} else
			gs.move_x = gs.move_y = 0;
		break;
	case SFVTCA0:
		dprintf0( "SFVTCA0");
		gs.f_vec_x = 0;
		gs.f_vec_y = UNITY2D14;
		gs.move_x = 0;
		if( gs.p_vec_y) {
			gs.move_y = (UNITY2D14<<14) / gs.p_vec_y;
			gs.flags = Y_TOUCHED;
		} else
			gs.move_y = gs.flags = 0;
		break;
	case SFVTCA1:
		dprintf0( "SFVTCA1");
		gs.f_vec_x = UNITY2D14;
		gs.f_vec_y = 0;
		if( gs.p_vec_x) {
			gs.move_x = (UNITY2D14<<14) / gs.p_vec_x;
			gs.flags = X_TOUCHED;
		} else
			gs.move_x = gs.flags = 0;
		gs.move_y = 0;
		break;
	case SDPVTL0:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SDPVTL0 p[%d] p[%d]", m, n);
		oldLine2vector( gs.zp1[n], gs.zp2[m], gs.dp_vec_x, gs.dp_vec_y);
		newLine2vector( gs.zp1[n], gs.zp2[m], gs.p_vec_x, gs.p_vec_y);
		gs.recalc();
		break;
	case SDPVTL1:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SDPVTL1 p[%d] p[%d]", m, n);
		oldLine2vector( gs.zp1[n], gs.zp2[m], gs.dp_vec_y, gs.dp_vec_x);
		gs.dp_vec_x = -gs.dp_vec_x;
		newLine2vector( gs.zp1[n], gs.zp2[m], gs.p_vec_y, gs.p_vec_x);
		gs.p_vec_x = -gs.p_vec_x;
		gs.recalc();
		break;
	case SPVTL0:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SPVTL0 p[%d] p[%d]", m, n);
		newLine2vector( gs.zp1[ n], gs.zp2[ m], gs.p_vec_x, gs.p_vec_y);
		gs.dp_vec_x = gs.p_vec_x;
		gs.dp_vec_y = gs.p_vec_y;
		gs.recalc();
		break;
	case SPVTL1:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SPVTL1 p[%d] p[%d]\t", m, n);
		newLine2vector( gs.zp1[n], gs.zp2[m], gs.p_vec_y, gs.p_vec_x);
		gs.dp_vec_y = gs.p_vec_y = -gs.p_vec_y;
		gs.dp_vec_x = gs.p_vec_x;
		gs.recalc();
		break;
	case SFVTL0:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SFVTL0 p[%d] p[%d]\t", m, n);
		newLine2vector( gs.zp1[n], gs.zp2[m], gs.f_vec_x, gs.f_vec_y);
		gs.recalc();
		break;
	case SFVTL1:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "SFVTL1 p[%d] p[%d]", m, n);
		newLine2vector( gs.zp1[n], gs.zp2[m], gs.f_vec_y, gs.f_vec_x);
		gs.f_vec_y = -gs.f_vec_y;
		gs.recalc();
		break;
	case SFVTPV:
		dprintf0( "SFVTPV");
		gs.f_vec_x = gs.p_vec_x; 
		gs.f_vec_y = gs.p_vec_y; 
		gs.recalc();
		break;
	case SPVFS:
		gs.dp_vec_y = gs.p_vec_y = *(stack--);
		gs.dp_vec_x = gs.p_vec_x = *(stack--);
		dprintf2( "#SPVFS = %d %d", gs.p_vec_x, gs.p_vec_y);
		gs.recalc();
		break;
	case SFVFS:
		gs.f_vec_y = *(stack--);
		gs.f_vec_x = *(stack--);
		dprintf2( "#SFVFS = %d %d", gs.f_vec_x, gs.f_vec_y);
		gs.recalc();
		break;
	case GPV:
		*(++stack) = gs.p_vec_x;
		*(++stack) = gs.p_vec_y;
		dprintf2( "GPV = %d %d", gs.p_vec_x, gs.p_vec_y);
		break;
	case GFV:
		*(++stack) = gs.f_vec_x;
		*(++stack) = gs.f_vec_y;
		dprintf2( "GFV = %d %d", gs.f_vec_x, gs.f_vec_y);
		break;
	case SRP0:
		gs.rp0 = *(stack--);
		dprintf1( "SRP0 p[%d]", gs.rp0);
		break;
	case SRP1:
		gs.rp1 = *(stack--);
		dprintf1( "SRP1 p[%d]", gs.rp1);
		break;
	case SRP2:
		gs.rp2 = *(stack--);
		dprintf1( "SRP2 p[%d]", gs.rp2);
		break;
	case SZP0:
		m = *(stack--);
		assert( m >= 0 && m <= 1);
		gs.zp0 = p[ m];
		dprintf1( "SZP0 %d", m);
		break;
	case SZP1:
		m = *(stack--);
		assert( m >= 0 && m <= 1);
		gs.zp1 = p[ m];
		dprintf1( "SZP1 %d", m);
		break;
	case SZP2:
		m = *(stack--);
		assert( m >= 0 && m <= 1);
		gs.zp2 = p[ m];
		dprintf1( "SZP2 %d", m);
		break;
	case SZPS:
		m = *(stack--);
		dprintf1( "SZPS %d", m);
		assert( m >= 0 && m <= 1);
		gs.zp2 = gs.zp1 = gs.zp0 = p[ m];
		break;
	case RTHG:
		dprintf0( "RTHG");
		gs.round_state	= ROUND_HALF;
		break;
	case RTG:
		dprintf0( "RTG");
		gs.round_state	= ROUND_GRID;
		break;
	case RTDG:
		dprintf0( "RTDG");
		gs.round_state	= ROUND_DOUBLE;
		break;
	case RDTG:
		dprintf0( "RDTG");
		gs.round_state	= ROUND_DOWN;
		break;
	case RUTG:
		dprintf0( "RUTG");
		gs.round_state	= ROUND_UP;
		break;
	case ROFF:
		dprintf0( "ROFF");
		gs.round_state	= ROUND_OFF;
		break;
	case SROUND:
		m = *(stack--);
		dprintf3( "SROUND %d %d %d", (m>>6)&3, (m>>4)&3, m);
		gs.round_state = ROUND_SUPER;
		n = (m >> 6) & 3;
		gs.round_period = 0x20 << n;
		n = (m >> 4) & 3;
		if( n == 3) gs.round_phase = 48;
		else gs.round_phase = (gs.round_period >> 2) * n;
		m &= 0x0F;
		if( m)
			gs.round_thold = (gs.round_period >> 3) * (m - 4);
		else
			gs.round_thold = gs.round_period - 1;
		dprintf3( "-> period 0x%02X, thold 0x%02X, phase 0x%02X",
			gs.round_period, gs.round_thold, gs.round_phase);
		break;
	case S45ROUND:
		m = *(stack--);
		dprintf3( "SROUND45 %d %d %d", (m>>6)&3, (m>>4)&3, m);
		gs.round_state = ROUND_SUPER45;
		gs.round_period = 1444 >> (7 - ((m>>6) & 3));
		gs.round_phase = (gs.round_period * (m & 0x30)) >> 6;
		m &= 0x0F;
		if( m)
			gs.round_thold = (gs.round_period * (m - 4)) >> 3;
		else
			gs.round_thold = gs.round_period - 1;
		dprintf3( "-> period 0x%02X, thold 0x%02X, phase 0x%02X",
			gs.round_period, gs.round_thold, gs.round_phase);
		break;
	case SLOOP:
		gs.loop = *(stack--);
		dprintf1( "SLOOP %d", gs.loop);
		break;
	case SMD:
		gs.min_distance = *(stack--);
		dprintf1( "SMD %d", gs.min_distance);
		break;
	case INSTCTRL:
		gs.instr_control = *(stack--);
		m = *(stack--);
		dprintf2( "###INSTCTRL %d %d", gs.instr_control, m);
		if( gs.instr_control == 1)
			if(m && grid_fitting >= 0) grid_fitting = -grid_fitting;
		break;
	case SCANCTRL:
		m = *(stack--);
		gs.dropout_control = 0;
		if( m & 0x0100 && mppem <= (m & 0xff))	gs.dropout_control = 1;
		if( m & 0x0200 && !xy && !yx)		gs.dropout_control = 1;
		if( m & 0x0400 && xx != yy)		gs.dropout_control = 1;
		if( m & 0x0800 && mppem > (m & 0xff))	gs.dropout_control = 0;
		if( m & 0x1000 && (xy || yx))		gs.dropout_control = 0;
		if( m & 0x2000 && xx == yy)		gs.dropout_control = 0;
		dprintf2( "SCANCTRL %04X -> %d", m, gs.dropout_control);
		break;
	case SCANTYPE:
		m = *(stack--);
		dprintf1( "###SCANTYPE %d", m);
		//### TODO
		break;
	case SCVTCI:
		gs.cvt_cut_in = *(stack--);
		dprintf1( "SCVTCI %d", gs.cvt_cut_in);
		break;
	case SSWCI:
		gs.swidth_cut_in = *(stack--);
		dprintf1( "SSWCI %d", gs.swidth_cut_in);
		break;
	case SSW:
		gs.swidth_value = *(stack--);
		dprintf1( "SSW %d", gs.swidth_value);
		break;
	case FLIPON:
		gs.auto_flip = 1;
		dprintf0( "FLIPON");
		break;
	case FLIPOFF:
		gs.auto_flip = 0;
		dprintf0( "FLIPOFF");
		break;
	case SANGW:
		//angle_weight is obsolete!
		m = *(stack--);
		dprintf1( "SANGW %d is obsolete", m);
		break;
	case SDB:
		gs.delta_base = *(stack--);
		dprintf1( "SDB %d", gs.delta_base);
		break;
	case SDS:
		gs.delta_shift = *(stack--);
		dprintf1( "SDS %d", gs.delta_shift);
		break;

	// do some measurements

	case GC0:
		pp = &gs.zp2[ *stack];
		dprintf2( "GC0 p[%d][%d]\t", gs.zp2==p[1], pp-gs.zp2);
		*stack = gs.absNewMeasure( pp->xnow, pp->ynow);
		dprintf1( "\t=> %d", *stack);
		break;
	case GC1:
		pp = &gs.zp2[ *stack];
		dprintf2( "GC1 p[%d][%d]\t", gs.zp2==p[1], pp-gs.zp2 );
		*stack = gs.absOldMeasure( pp->xold, pp->yold);
		dprintf1( "\t=> %d", *stack);
		break;
	case SCFS:
		// move point along freedom vector, so that
		// projection gets desired length
		m = *(stack--);
		n = *(stack--);
		dprintf3( "SCFS p[%d][%d] to %f", gs.zp2==p[1], n, m/FSHIFT);
		pp = &gs.zp2[ n];
		if( gs.zp2 == p[1]) {
			int i = gs.absNewMeasure( pp->xnow, pp->ynow);
			gs.movePoint( *pp, m - i);
		} else { // magic in the twilight zone
			pp->xold = pp->xnow = (m * gs.p_vec_x) >> 14;
			pp->yold = pp->ynow = (m * gs.p_vec_y) >> 14;
		}
		break;
	case MD0:
		m = *(stack--);
		n = *stack;
		dprintf2( "MD0 p[%d][%d] ", gs.zp1==p[1], m);
		dprintf2( "- p[%d][%d]", gs.zp0==p[1], n);
		*stack = newMeasure( gs.zp0[ n], gs.zp1[ m]);
		break;
	case MD1:
		m = *(stack--);
		n = *stack;
		dprintf2( "MD1 p[%d][%d] ", gs.zp1==p[1], m);
		dprintf2( "- p[%d][%d]", gs.zp0==p[1], n);
		*stack = oldMeasure( gs.zp0[ n], gs.zp1[ m]); // thanks David
		break;
	case MPPEM:
		dprintf0( "MPPEM\t");
		m = gs.absNewMeasure( mppemx, mppemy);
		if( m < 0) m = -m;
		*(++stack) = m;
		dprintf1( "\t => mppem = %d", m);
		break;
	case MPS:
		*(++stack) = pointSize;
		dprintf1( "MPS %d", *stack);
		break;

	// outline manipulation

	case FLIPPT:
		for( m = gs.loop; --m >= 0;) {
			n = *(stack--);
			dprintf3( "FLIPPT * %d p[%d][%d]", m, gs.zp0==p[1], n);
			gs.zp1[ n].flags ^= ON_CURVE;
		}
		gs.loop = 1;
		break;
	case FLIPRGON:
		m = *(stack--);
		n = *(stack--);
		dprintf3( "FLIPRGON p[%d][ %d .. %d]", gs.zp0==p[1], n, m);
		pp = &gs.zp1[ n];
		for( m -= n-1; --m >= 0; ++pp)
			pp->flags |= ON_CURVE;
		break;
	case FLIPRGOFF:
		m = *(stack--);
		n = *(stack--);
		dprintf3( "FLIPRGOFF p[%d][ %d .. %d]", gs.zp0==p[1], n, m);
		pp = &gs.zp1[ n];
		for( m -= n-1; --m >= 0; ++pp)
			pp->flags &= ~ON_CURVE;
		break;
	case SHP0:
	case SHP1:
		pp = (opc & 1) ? &gs.zp0[ gs.rp1] : &gs.zp1[ gs.rp2];
		n = gs.absNewMeasure( pp->xnow - pp->xold, pp->ynow - pp->yold);
		for( m = gs.loop; --m >= 0;) {
			int i = *(stack--);
			dprintf3( "SHP * %d p[%d], rp = p[%d]", m,i, pp-p[1]);
			dprintf1( " moved by %f", n/FSHIFT);
			gs.movePoint( gs.zp2[ i], n);
		}
		gs.loop = 1;
		break;
	case SHC0:
	case SHC1:
		{
		m = *(stack--);
		assert( m >= 0 && m < sizeContours);
		pp = (opc & 1) ? &gs.zp0[ gs.rp1] : &gs.zp1[ gs.rp2];
		dprintf2( "SHC%d rp[%d]", opc & 1, pp - p[1]);
		n = gs.absNewMeasure( pp->xnow - pp->xold, pp->ynow - pp->yold);
		int i = (m <= 0) ? 0 : endPoints[ m-1] + 1;
		m = (gs.zp2==p[0]) ? nPoints[0] : endPoints[ m];
		for( ; i <= m; ++i) {
			if( pp == &gs.zp2[ i])
				continue;
			dprintf3( "SHC%d p[%d] by %f\n", opc&1, i, n/FSHIFT);
			gs.movePoint( gs.zp2[ i], n);
		}
		}
		break;
	case SHZ0:
	case SHZ1:
		{
		m = *(stack--);
		dprintf2( "SHZ%d rp = p[%d]\n ", opc&1, (opc&1)?gs.rp1:gs.rp2);
		pp = (opc & 1) ? &gs.zp0[ gs.rp1] : &gs.zp1[ gs.rp2];
		n = gs.absNewMeasure( pp->xnow - pp->xold, pp->ynow - pp->yold);
		assert( m >= 0 && m <= 1);
		for( point *pp1= p[m], *pp2= pp1+nPoints[m]; pp1 < pp2; ++pp1) {
			if( pp1 == pp) continue;
			dprintf2( "\nSHZ p[%d] by %f", pp1-p[m], n/FSHIFT);
			dprintf2( "\t(%d %d) -> ", pp1->xnow, pp1->ynow);
			pp1->xnow += (n * gs.move_x) >> 14;
			pp1->ynow += (n * gs.move_y) >> 14;
			dprintf2( "(%d %d)\n", pp1->xnow, pp1->ynow);
		}
		}
		break;
	case SHPIX:
		m = *(stack--);
		for( n = gs.loop; --n >= 0;) {
			int i = *(stack--);
			dprintf3( "SHPIX * %d p[%d][%d] ", n, gs.zp2==p[1], i);
			dprintf1( "by %f", m/FSHIFT);
			pp = &gs.zp2[ i];
			dprintf2( "\n%d %d ->", pp->xnow, pp->ynow);
			pp->xnow += (m * gs.f_vec_x) >> 14;
			pp->ynow += (m * gs.f_vec_y) >> 14;
			pp->flags |= gs.flags;
			dprintf2( "\t%d %d", pp->xnow, pp->ynow);
		}
		gs.loop = 1;
		break;
	case MSIRP0:
	case MSIRP1:
		m = *(stack--);
		n = *(stack--);
		gs.rp2 = n;
		gs.rp1 = gs.rp0;
		dprintf3( "MSIRP%d p[%d][%d] ", opc&1, gs.zp1==p[1], n);
		dprintf3( "to %f, rp = p[%d][%d]", m/FSHIFT, gs.zp0==p[1], gs.rp0);
		if( gs.zp1 == p[1]) {
			int i = newMeasure( p[1][n], gs.zp0[ gs.rp0]);
			gs.movePoint( p[1][n], m-i);
		} else {	// magic in the twilight zone
			pp = &p[0][n];
			pp->xnow = pp->xold =
				((m * gs.p_vec_x) >> 14) + gs.zp0[ gs.rp0].xnow;
			pp->ynow = pp->yold =
				((m * gs.p_vec_y) >> 14) + gs.zp0[ gs.rp0].ynow;
		}
		if( opc & 1)
			gs.rp0 = n;
		break;
	case MDAP0:
	case MDAP1:
		gs.rp0 = gs.rp1 = *(stack--);
		dprintf2( "MDAP%d p[%d]", opc&1, gs.rp0);
		pp = &gs.zp0[ gs.rp0];
		dprintf2( "\nxy %d %d", pp->xnow, pp->ynow);
		pp->flags |= gs.flags;
		if( opc & 1) {
#if 0 //###
			if( gs.f_vec_x)
				pp->xnow = round( pp->xnow);
			if( gs.f_vec_y)
				pp->ynow = round( pp->ynow);
			dprintf2( "\t-> %d %d", pp->xnow, pp->ynow);
#else
			m = gs.absNewMeasure( pp->xnow, pp->ynow);
			gs.movePoint( *pp, round(m) - m);
			dprintf2( "\t-> %d %d", pp->xnow, pp->ynow);
#endif
		}
		break;
	case MIAP0:
	case MIAP1:
		m = *(stack--);
		gs.rp0 = gs.rp1 = *(stack--);
		dprintf3( "MIAP%d p[%d][%d] ", opc&1, gs.zp0==p[1], gs.rp0);
		dprintf1( "to cvt[%d] = ", m);
		m = cvt[ m];
		dprintf1( "%f", m/FSHIFT);
		if( gs.zp0 != p[0]) {
			pp = &p[1][gs.rp0];
			int i = gs.absNewMeasure( pp->xnow, pp->ynow);
			if( opc & 0x01) {
				if( ((m>i) ? m-i : i-m) > gs.cvt_cut_in)
					m = i;
				m = round( m);
			}
			dprintf1( "\nabsdist = %f", i/FSHIFT);
			gs.movePoint( gs.zp0[ gs.rp0], m - i);
		} else {	// magic in the twilight zone
			pp = &p[0][gs.rp0];
			pp->xold = pp->xnow = (m * gs.p_vec_x) >> 14;
			pp->yold = pp->ynow = (m * gs.p_vec_y) >> 14;
			if( opc & 0x01) {
				pp->xnow = round( pp->xnow);
				pp->ynow = round( pp->ynow);
			}
		}
		break;
	case MDRP00: case MDRP01:
	case MDRP02: case MDRP03:
	case MDRP04: case MDRP05:
	case MDRP06: case MDRP07:
	case MDRP08: case MDRP09:
	case MDRP0A: case MDRP0B:
	case MDRP0C: case MDRP0D:
	case MDRP0E: case MDRP0F:
	case MDRP10: case MDRP11:
	case MDRP12: case MDRP13:
	case MDRP14: case MDRP15:
	case MDRP16: case MDRP17:
	case MDRP18: case MDRP19:
	case MDRP1A: case MDRP1B:
	case MDRP1C: case MDRP1D:
	case MDRP1E: case MDRP1F:
		gs.rp2 = *(stack--);
		gs.rp1 = gs.rp0;
		dprintf3( "#MDRP%02X p[%d], rp = p[%d]", opc&15, gs.rp2,gs.rp0);
		n = oldMeasure( gs.zp1[ gs.rp2], gs.zp0[ gs.rp0]);
		m = newMeasure( gs.zp1[ gs.rp2], gs.zp0[ gs.rp0]);
		dprintf2( "\nwgoaldist = %f, nowdist = %f", n/FSHIFT, m/FSHIFT);
		dprintf2( "\n(%d %d)-",gs.zp1[gs.rp2].xnow,gs.zp1[gs.rp2].ynow);
		dprintf2( "rp0(%d %d)",gs.zp0[gs.rp0].xnow,gs.zp0[gs.rp0].ynow);

		if( ((n >= 0) ? +n : -n) < gs.swidth_cut_in)
			n = (n >= 0) ? +gs.swidth_value : -gs.swidth_value;
		if( opc & 0x10)
			gs.rp0 = gs.rp2;
		dprintf1( "\nmdrp1.wanted = %d", n);
		if( opc & 0x08)
			if( n >= 0) {
				if( n < +gs.min_distance)
					n = +gs.min_distance;
			} else {
				if( n > -gs.min_distance)
					n = -gs.min_distance;
			}
		if( opc & 0x04)
			n = round( n);
		dprintf1( "\nmdrp2.wanted = %d", n);
		//### ignore black/gray/white for now
		gs.movePoint( gs.zp1[ gs.rp2], n - m);
		break;
	case MIRP00: case MIRP01:
	case MIRP02: case MIRP03:
	case MIRP04: case MIRP05:
	case MIRP06: case MIRP07:
	case MIRP08: case MIRP09:
	case MIRP0A: case MIRP0B:
	case MIRP0C: case MIRP0D:
	case MIRP0E: case MIRP0F:
	case MIRP10: case MIRP11:
	case MIRP12: case MIRP13:
	case MIRP14: case MIRP15:
	case MIRP16: case MIRP17:
	case MIRP18: case MIRP19:
	case MIRP1A: case MIRP1B:
	case MIRP1C: case MIRP1D:
	case MIRP1E: case MIRP1F:
		gs.rp1 = gs.rp0;
		m = *(stack--);
		gs.rp2 = *(stack--);
		pp = &gs.zp1[ gs.rp2];
		dprintf3( "#MIRP%02X p[%d] with cvt[%d]", opc&15, gs.rp2, m);

		m = cvt[ m];
		dprintf2( " = %f, rp = p[%d]", m/FSHIFT, gs.rp0);
		if( ((m >= 0)? +m : -m) < +gs.swidth_cut_in)
			m = (m >= 0) ? +gs.swidth_value : -gs.swidth_value;

		n = oldMeasure( *pp, gs.zp0[gs.rp0]);

		if( (n^m) < 0 && gs.auto_flip) {
			m = -m;
			dprintf1( "\nautoflip m = %f", m/FSHIFT);
		}
		if( opc & 0x04) {
			if( ((m>n) ? m-n : n-m) >= +gs.cvt_cut_in)
				m = n;
			m = round( m);
		}
		if( opc & 0x08) {
			if( n >= 0) {
				if( m < +gs.min_distance)
					m = +gs.min_distance;
 			} else {
				if( m > -gs.min_distance)
					m = -gs.min_distance;
			}
		}
		//### ignore black/gray/white for now
		m -= newMeasure( *pp, gs.zp0[gs.rp0]);
		gs.movePoint( *pp, m);
		if( opc & 0x10)
			gs.rp0 = gs.rp2;
		break;
	case ALIGNRP:
		for( m = gs.loop; --m >= 0;) {
			int n = *(stack--);
			dprintf3( "ALIGNRP * %d p[%d], rp0 = p[%d]",m,n,gs.rp0);
			int i = newMeasure( gs.zp0[ gs.rp0], gs.zp1[ n]);
			gs.movePoint( gs.zp1[ n], i);
		}
		gs.loop = 1;
		break;
	case ALIGNPTS:
		{
		m = *(stack--);
		n = *(stack--);
		dprintf2( "ALIGNPTS %d %d", m, n);
		int i = newMeasure( gs.zp0[ m], gs.zp1[ n]) >> 1;
		gs.movePoint( gs.zp0[ m], -i);
		gs.movePoint( gs.zp1[ n], +i);
		}
		break;
	case ISECT:
		{
		point* pp1 = &gs.zp1[ *(stack--)];
		point* pp2 = &gs.zp1[ *(stack--)];
		point* pp3 = &gs.zp0[ *(stack--)];
		point* pp4 = &gs.zp0[ *(stack--)];
		m = *(stack--);

		dprintf1( "ISECT p[%d] ", m);
		dprintf2( "between p[%d]-p[%d] ", pp1-gs.zp1, pp2-gs.zp1);
		dprintf2( "and p[%d]-p[%d] ", pp3-gs.zp0, pp4-gs.zp0);

		int f1 = (pp1->xnow - pp3->xnow) * (pp4->ynow - pp3->ynow)
			-(pp1->ynow - pp3->ynow) * (pp4->xnow - pp3->xnow);
		int f2 = (pp2->ynow - pp1->ynow) * (pp4->xnow - pp3->xnow)
			-(pp2->xnow - pp1->xnow) * (pp4->ynow - pp3->ynow);

		pp3 = &gs.zp2[ m];
		pp3->flags |= X_TOUCHED | Y_TOUCHED;
		if( f2 == 0 ) { // parallel => no intersection
			pp3->xnow = (pp2->xnow + pp1->xnow + 1) >> 1;
			pp3->ynow = (pp2->ynow + pp1->ynow + 1) >> 1;
			dprintf0( "are parallel!\n");
		} else {
			pp3->xnow = pp1->xnow
				+ MULDIV( f1, pp2->xnow - pp1->xnow, f2);
			pp3->ynow = pp1->ynow
				+ MULDIV( f1, pp2->ynow - pp1->ynow, f2);
		}

		dprintf2( "\n-> %d %d", pp3->xnow, pp3->ynow);
		}
		break;
	case AA:
		stack--;
		dprintf0( "AA is obsolete and not supported!");
		break;
	case IP:
		for( m = gs.loop; --m >= 0;) {
			int n = *(stack--);
			dprintf2( "IP * %d p[%d] ", m, n);
			dprintf2( "between p[%d][%d] ", gs.zp1==p[1], gs.rp2);
			dprintf2( "and p[%d][%d]", gs.zp0==p[1], gs.rp1);
			interpolate( gs.zp2[ n],
				gs.zp1[ gs.rp2], gs.zp0[ gs.rp1]);
			dprintf0( "\n");
		}
		gs.loop = 1;
		break;
	case UTP:
		m = *(stack--);
		gs.zp0[ m].flags &= ~(X_TOUCHED | Y_TOUCHED);
		dprintf1( "UTP p[%d]", m);
		break;
	case IUP0:
		pp = p[1];
		for( m = 0; m < nEndPoints; ++m) {
			point* last = p[1] + endPoints[ m];
			dprintf2( "IUP0 p[%d .. %d]", pp-p[1], last-p[1]);
			doIUP0( pp, last);
			pp = last + 1;
		}
		break;
	case IUP1:
		pp = p[1];
		for( m = 0; m < nEndPoints; ++m) {
			point* last = p[1] + endPoints[ m];
			dprintf2( "IUP1 p[%d .. %d]", pp-p[1], last-p[1]);
			doIUP1( pp, last);
			pp = last + 1;
		}
		break;
	case DELTAP3:
		n = -32;
		goto deltap_label;
	case DELTAP2:
		n = -16;
		goto deltap_label;
	case DELTAP1:
		n = 0;
deltap_label:
		m = *(stack--);
		dprintf2( "DELTAP%d * %d", (-n >> 4) + 1, m);
		dprintf2( "\tmppem=%d, deltabase=%d", mppem, gs.delta_base);
		n += mppem - gs.delta_base;
		if( n < 0 || n > 15) {
			dprintf1( "\n=> skipping %d exceptions", m);
			stack -= m << 1;
			break;
		}
		n <<= 4;
		while( --m >= 0) {
			int pno = *(stack--);
			int arg = *(stack--);
			dprintf2( "\np[%d] arg %04X", pno, arg);
			dprintf2( "\targ.n=%d, n=%d", arg>>4, n>>4);
			if( n > (arg & 0xf0))
				break;
			if( n == (arg & 0xf0)) {
				arg = (arg & 15) - 8;
				if( arg >= 0) ++arg;
				arg <<= (SHIFT - gs.delta_shift);
				dprintf3( "\tmoving by %f from (%d %d) ",
					arg/FSHIFT,
					gs.zp0[ pno].xnow, gs.zp0[ pno].ynow);
#if 0
				gs.movePoint( gs.zp0[ pno], arg);
#else
				gs.zp0[ pno].xnow += (arg * gs.f_vec_x) >> 14;
				gs.zp0[ pno].ynow += (arg * gs.f_vec_y) >> 14;
				gs.zp0[ pno].flags |= gs.flags;
#endif
				dprintf2("to (%d %d)\n",
					gs.zp0[ pno].xnow, gs.zp0[ pno].ynow);
			}
		}

#ifndef DEBUG
		if( m > 0)
			stack -= 2 * m;
#else
		while( --m >= 0) {
			int pno = *(stack--);
			int arg = *(stack--);
			dprintf2( "\n(p[%d] arg %04X", pno, arg);
			dprintf2( "\targ.n=%d, n=%d)", arg>>4, n>>4);
		};
#endif
		dprintf0( "\n");
		break;

	case DELTAC3:
		n = -32;
		goto deltac_label;
	case DELTAC2:
		n = -16;
		goto deltac_label;
	case DELTAC1:
		n = 0;
deltac_label:
		m = *(stack--);
		dprintf2( "DELTAC%d * %d", (-n >> 4) + 1, m);
		dprintf2( "\tmppem=%d, deltabase=%d", mppem, gs.delta_base);
		n += mppem - gs.delta_base;
		if( n < 0 || n > 15) {
			stack -= m << 1;
			break;
		}
		n <<= 4;
		while( --m >= 0) {
			int cno = *(stack--);
			int arg = *(stack--);
			dprintf3( "\ncvt[%d] arg = %04X, n = %d", cno, arg, n>>4);
			if( n > (arg & 0xf0))
				break;
			if( n == (arg & 0xf0)) {
				arg = (arg & 15) - 8;
				if( arg >= 0) ++arg;
				arg <<= SHIFT - gs.delta_shift;
				dprintf2( "\tmoved by %f,\t%d ", arg/FSHIFT, cvt[cno]);
				cvt[ cno] += arg;
				dprintf1( "-> %d", cvt[cno]);
			}
		}
#ifndef DEBUG
		if( m > 0)
			stack -= 2 * m;
#else
		while( --m >= 0) {
			int cno = *(stack--);
			int arg = *(stack--);
			dprintf2( "\n(cvt[%d] arg %04X", cno, arg);
			dprintf2( "\targ.n=%d, n=%d)", arg>>4, n>>4);
		};
#endif
		break;

	// stack manipulation

	case DUP:
		m = *stack;
		*(++stack) = m;
		dprintf1( "DUP = %d", m);
		break;
	case POP:
		dprintf0( "POP");
		--stack;
		break;
	case CLEAR:
		dprintf0( "CLEAR");
		stack = stackbase;
		break;
	case SWAP:
		dprintf0( "SWAP");
		m = *stack;
		*stack = *(stack-1);
		*(stack-1) = m;
		break;
	case DEPTH:
		m = stack - stackbase;
		*(++stack) = m;
		dprintf1( "DEPTH = %d", m);
		break;
	case CINDEX:
		m = *stack;
		assert( stack-m >= stackbase);
		*stack = *(stack - m);
		dprintf2( "CINDEX %d = %d", m, *stack);
		break;
	case MINDEX:
		m = *stack;
		stack -= m;
		assert( stack >= stackbase);
		n = *stack;
		dprintf2( "MINDEX %d = %d", m, n);
		for( ; --m > 0; ++stack)
			stack[0] = stack[1];
		*stack = n;
		break;
	case ROLL:
		m = *(stack - 0);
		*(stack - 0) = *(stack - 2);
		*(stack - 2) = *(stack - 1);
		*(stack - 1) = m;
		dprintf3( "ROLL %d %d %d", m, *(stack-2), *stack);
		dprintf3( " => %d %d %d", *stack, m, *(stack-2));
		break;

	// control flow

	case IF:
		m = *(stack--);
		dprintf1( "IF %d", m);
		if( !m)
			skipHints( f);
		break;
	case ELSE:
		// if we hit ELSE we didn't skip -> skip from here
		dprintf0( "ELSE");
		skipHints( f);
		break;
	case EIF:
		dprintf0( "EIF");
		break;
	case JROT:
		m = *(stack--);
		dprintf1( "JROT %d -> ", m);
		if( m)
			goto jump_relative;
		dprintf0( "not taken");
		--stack;
		break;
	case JROF:
		m = *(stack--);
		dprintf1( "JROF %d -> ", m);
		if( !m)
			goto jump_relative;
		dprintf0( "not taken");
		--stack;
		break;
	case JMPR:
jump_relative:
		m = *(stack--);
		dprintf1( "JMPR %d", m);
		f->seekRelative( m-1);
		break;
	case LT:
		m = *(stack--);
		n = *stack;
		*stack = (n < m);
		dprintf3( "LT %d %d = %d", m, n, *stack);
		break;
	case LTEQ:
		m = *(stack--);
		n = *stack;
		*stack = (n <= m);
		dprintf3( "LTEQ %d %d = %d", m, n, *stack);
		break;
	case GT:
		m = *(stack--);
		n = *stack;
		*stack = (n > m);
		dprintf3( "GT %d %d = %d", m, n, *stack);
		break;
	case GTEQ:
		m = *(stack--);
		n = *stack;
		*stack = (n >= m);
		dprintf3( "GTEQ %d %d = %d", m, n, *stack);
		break;
	case EQ:
		m = *(stack--);
		n = *stack;
		*stack = (m == n);
		dprintf3( "EQ %d %d = %d", m, n, *stack);
		break;
	case NEQ:
		m = *(stack--);
		n = *stack;
		*stack = (m != n);
		dprintf3( "NEQ %d %d = %d", m, n, *stack);
		break;
	case ODD:
		m = *stack;
		*stack = (round(m) >> SHIFT) & 1;
		dprintf2( "ODD %d = %d", m, *stack);
		break;
	case EVEN:
		m = *stack;
		*stack = ((~round(m)) >> SHIFT) & 1;
		dprintf2( "EVEN %d = %d", m, *stack);
		break;
	case AND:
		m = *(stack--);
		n = *stack;
		*stack = n && m;
		dprintf3( "AND %d %d = %d", m, n, *stack);
		break;
	case OR:
		m = *(stack--);
		n = *stack;
		*stack = n || m;
		dprintf3( "OR %d %d = %d", m, n, *stack);
		break;
	case NOT:
		m = *stack;
		*stack = !m;
		dprintf2( "NOT %d = %d", m, *stack);
		break;
	case ADD:
		m = *(stack--);
		*stack += m;
		dprintf3( "ADD %d %d = %d", m, *stack-m, *stack);
		break;
	case SUB:
		m = *(stack--);
		*stack -= m;
		dprintf3( "SUB %d %d = %d", m, *stack+m, *stack);
		break;
	case DIV:
		m = *(stack--);
		n = *stack;
		if( m)	*stack = (n << SHIFT) / m;
		else	*stack = (n>=0) ? 0x7ffffff : -0x7ffffff;
		dprintf3( "DIV %d %d = %d", m, n, *stack);
		break;
	case MUL:
		m = *(stack--);
		n = *stack;
		*stack = (m * n + 32) >> SHIFT;
		dprintf3( "MUL %d %d = %d", m, n, *stack);
		break;
	case ABS:
		m = *stack;
		if( m < 0) *stack = -m;
		dprintf2( "ABS %d = %d", m, *stack);
		break;
	case NEG:
		*stack = -*stack;
		dprintf2( "NEG %d = %d", -*stack, *stack);
		break;
	case FLOOR:
		m = *stack;
		*stack = m & -64;
		dprintf2( "FLOOR %d = %d", m, *stack);
		break;
	case CEILING:
		m = *stack;
		*stack = (m + 63) & -64;
		dprintf2( "CEILING %d = %d", m, *stack);
		break;
	case MAX:
		m = *(stack--);
		n = *stack;
		if( m > n) *stack = m;
		dprintf3( "MAX %d %d = %d", m, n, *stack);
		break;
	case MIN:
		m = *(stack--);
		n = *stack;
		if( m < n) *stack = m;
		dprintf3( "MIN %d %d = %d", m, n, *stack);
		break;
	case ROUND00: case ROUND01:
	case ROUND02: case ROUND03:
		// 00: gray, 01: black, 02: white, 03: ???
		m = *stack;
		//### ignore black/gray/white for now
		*stack = round( m);
		dprintf3( "#ROUND%02X %d = %d", opc&3, m, *stack);
		break;
	case NROUND00: case NROUND01:
	case NROUND02: case NROUND03:
		// 00: gray, 01: black, 02: white, 03: ???
		m = *stack;
		//### ignore black/gray/white for now
		*stack = m;
		dprintf3( "#NROUND%02X %d = %d", opc&3, m, *stack);
		break;
	case FDEF:
		m = *(stack--);
		dprintf1( "FDEF %d", m);
		assert( m >= 0 && m < sizeFDefs);
		fdefs[m].f	= f;
		fdefs[m].offset	= f->tell();
		skipHints( f);
		fdefs[m].length	= f->tell() - fdefs[ m].offset;
		break;
	case ENDF:
		dprintf0( "ENDF\n");
		return;
	case IDEF:
		m = *(stack--);
		dprintf1( "IDEF %02X", m);
		assert( m >= 0 && m < sizeIDefs);
		idefs[m].f	= f;
		idefs[m].offset	= f->tell();
		skipHints( f);
		idefs[m].length	= f->tell() - idefs[ m].offset;
		break;
	case CALL:
		m = *(stack--);
		dprintf1( "CALL %d\n", m);
		assert( m >= 0 && m < sizeFDefs);
		{
			int ofs = f->tell();
			FDefs* fd = &fdefs[m];
			execHints( fd->f, fd->offset, fd->length);
			f->seekAbsolute( ofs);
		}
		break;
	case LOOPCALL:
		m = *(stack--);
		n = *(stack--);
		dprintf2( "LOOPCALL %d * %d\n", m, n);
		{
			int ofs = f->tell();
			FDefs* fd = &fdefs[m];
			while( --n >= 0)
				execHints( fd->f, fd->offset, fd->length);
			f->seekAbsolute( ofs);
		}
		break;
	case DBG:
		dprintf0( "DBG not implemented");
		break;
	case GETINFO:
		m = *stack;
		*stack = 0;
		if( m & SCALER_VERSION)
			*stack = WIN_SCALER;
		if( m & GLYPH_ROTATED)
			if( xy | yx)
				*stack |= IS_ROTATED;
		if( m & GLYPH_STRETCHED)
			if( xx != yy)
				*stack |= IS_STRETCHED;
		dprintf2( "GETINFO %d = 0x%03X", m, *stack);
		break;
	default:
		{
			int ofs = f->tell();
			IDefs* idef = &idefs[ opc];
			dprintf3( "IDEF_CALL 0x%02X, ofs = %05X, len = %d\n",
				opc, idef->offset, idef->length);
			if( idef->length)	// thanks Colin McCormack
				execHints( idef->f, idef->offset, idef->length);
			else
				dprintf1( "illegal instruction %02X\n", opc);
			f->seekAbsolute( ofs);
		}
		break;
	}
}


void Rasterizer::hintGlyph( GlyphTable* g, int offset, int length)
{
	if( grid_fitting <= 0 || length == 0)
		return;

	//###gs = default_gs;
	gs.init( p);
	gs.cvt_cut_in = default_gs.cvt_cut_in;
	gs.zp2 = gs.zp1 = gs.zp0 = p[1];
	stack = stackbase;
	execHints( g, offset, length);
}


void Rasterizer::execHints( RandomAccessFile* const f, int offset, int length)
{
	assert( 0 <= length && length <= 10000);

	f->seekAbsolute( offset);
	for( length += offset; f->tell() < length;)
		execOpcode( f);
	dprintf0( "\n\n");
}


void Rasterizer::skipHints( RandomAccessFile* const f)
{
	dprintf0( "\nskipping...");
	for( int depth = 0;;) {
		int opc = f->readUByte();
		dprintf1( " %02X ", opc);
		switch( opc) {
		case NPUSHB:
			opc = f->readUByte() + PUSHB00 - 1;
			// fall through
		case PUSHB00: case PUSHB01:
		case PUSHB02: case PUSHB03:
		case PUSHB04: case PUSHB05:
		case PUSHB06: case PUSHB07:
			f->seekRelative( opc - (PUSHB00 - 1));
			break;
		case NPUSHW:
			opc = f->readUByte() + PUSHW00 - 1;
			// fall through
		case PUSHW00: case PUSHW01:
		case PUSHW02: case PUSHW03:
		case PUSHW04: case PUSHW05:
		case PUSHW06: case PUSHW07:
			f->seekRelative( (opc - (PUSHW00 - 1)) << 1);
			break;
		case IF:
		case FDEF:
		case IDEF:
			++depth;
			break;
		case EIF:
		case ENDF:
			if( --depth < 0)
				return;
			break;
		case ELSE:
			if( depth <= 0)
				return;
			break;
		}
	}
}


void Rasterizer::interpolate( point& pp, const point& p2, const point& p1)
{
	// interpolate pp to satisfy
	//
	//   dist(pp, p1)        dist(pp, p2)
	// ---------------- == ----------------
	//  dist(pp', p1')      dist(pp', p2')

	int dold21 = oldMeasure( p2, p1);
	int doldp1 = oldMeasure( pp, p1);

	int dist;

	if( (dold21 ^ doldp1) < 0 || doldp1 == 0)
		dist = newMeasure(p1, pp) + doldp1; //- oldMeasure(p1, pp);
	else if( (dold21>=0 && doldp1>=dold21) || (dold21<=0 && doldp1<=dold21))
		dist = newMeasure(p2, pp) - oldMeasure(p2, pp);
	else {
		int dnew21 = newMeasure( p2, p1);
		int dnewp1 = newMeasure( pp, p1);
		dist = MULDIV( doldp1, dnew21, dold21) - dnewp1;
	}

	dprintf1( "\nmove by %f", dist/FSHIFT);
	gs.movePoint( pp, dist);
}


void Rasterizer::iup0( point* const pp,
	const point* const p1, const point* const p2)
{
	int dold21 = p2->yold - p1->yold;
	int doldp1 = pp->yold - p1->yold;

	dprintf3( "\np[%d] between p[%d] and p[%d]", pp-p[1], p1-p[1], p2-p[1]);
	dprintf2( "\nd21o dp1o %f %f", dold21/FSHIFT, doldp1/FSHIFT);

	dprintf2( "\tchanging y: %d %d", pp->xnow, pp->ynow);

	if( (dold21 ^ doldp1) < 0 || doldp1 == 0)
		pp->ynow = pp->yold + p1->ynow - p1->yold;
	else if( (dold21>=0 && doldp1>=dold21) || (dold21<=0 && doldp1<=dold21))
		pp->ynow = pp->yold + p2->ynow - p2->yold;
	else {
		int dnew21 = p2->ynow - p1->ynow;
		dprintf1( "\nd21n %8.3f", dnew21/FSHIFT);
		pp->ynow = MULDIV( doldp1+1, dnew21, dold21) + p1->ynow;
	}

	dprintf2( " -> %d %d\n", pp->xnow, pp->ynow);
}


void Rasterizer::iup1( point* const pp,
	const point* const p1, const point* const p2)
{
	int dold21 = p2->xold - p1->xold;
	int doldp1 = pp->xold - p1->xold;
	
	dprintf3( "\np[%d] between p[%d] and p[%d]",
		pp-p[1], p1-p[1], p2-p[1]);
	dprintf2( "\nd21o dp1o %f %f", dold21/FSHIFT, doldp1/FSHIFT);

	dprintf2( "\nchanging x: %d %d", pp->xnow, pp->ynow);

	if( (dold21 ^ doldp1) < 0 || doldp1 == 0)
		pp->xnow = pp->xold + p1->xnow - p1->xold;
	else if( (dold21>=0 && doldp1>=dold21) || (dold21<=0 && doldp1<=dold21))
		pp->xnow = pp->xold + p2->xnow - p2->xold;
	else {
		int dnew21 = p2->xnow - p1->xnow;
		dprintf1( "\t(d21n %8.3f)", dnew21/FSHIFT);
		pp->xnow = MULDIV( doldp1+1, dnew21, dold21) + p1->xnow;
	}

	dprintf2( " -> %d %d\n", pp->xnow, pp->ynow);
}


void Rasterizer::doIUP0( point* const first, point* const last)
{
	point* p0;
	for( p0 = first; p0 <= last; ++p0)
		if( p0->flags & Y_TOUCHED)
			break;
	point *i, *j;
	for( i = j = p0; i <= last; i = j) {
		while( ++j <= last)
			if( j->flags & Y_TOUCHED)
				break;
		if( j > last)
			break;
		for( point* k = i; ++k < j;)
			iup0( k, i, j);
	}
	if( i > last)
		return;
	for( j = i; ++j <= last;)
		iup0( j, i, p0);
	for( j = first; j < p0; ++j)
		iup0( j, i, p0);
}


void Rasterizer::doIUP1( point* const first, point* const last)
{
	point* p0;
	for( p0 = first; p0 <= last; ++p0)
		if( p0->flags & X_TOUCHED)
			break;
	point *i, *j;
	for( i = j = p0; i <= last; i = j) {
		while( ++j <= last)
			if( j->flags & X_TOUCHED)
				break;
		if( j > last)
			break;
		for( point* k = i; ++k < j;)
			iup1( k, i, j);
	}
	if( i > last)
		return;
	for( j = i; ++j <= last;)
		iup1( j, i, p0);
	for( j = first; j < p0; ++j)
		iup1( j, i, p0);
}


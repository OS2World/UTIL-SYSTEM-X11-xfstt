// Header for glyph remapping subsystem
// (C) Copyright 1998 Herbert Duerr

class Encoding
{
public:
	virtual ~Encoding() {}
	virtual int map2unicode( int code) = 0;
	virtual int hasGlyphs( int /*unicodeRange*/[4]) { return 1;}
	static void getDefault( Encoding** maps, int maxcodes);
	static int parse( char* mapnames, Encoding** maps, int maxcodes);
	static Encoding* find( char* mapname);
	static Encoding* enumerate( Encoding* iterator);
	
	const char* strName;
	const int lenName;

protected:
	Encoding( char* name);
private:
	static Encoding *first, *last;
	Encoding* next;
};


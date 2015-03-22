// quick and dirty hack to patch a ttf file and to fix the checksums

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define FONTDIR ""	//"/DOS/windows/system/"

typedef unsigned char U8;

void usage()
{
	printf( "Usage: patchttf fontfile -p 0xaddr 0xdata");
	// typical: CLEAR PUSHB00 0 IF ... EIF
	// => 22 B0 00 58 ... 59
}

int checksum( U8* buf, int len)
{
	len = (len + 3) >> 2;
	int sum = 0;
	for( U8* p = buf; --len >= 0; p += 4) {
		int val = (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3];
		sum += val;
	}
	return sum;
}

int patchttf( int argc, char** argv)
{
	char inTTname[160], outTTname[160];

	if( argc < 2)
		return -1;
	strcpy( inTTname, argv[1]);
	strcpy( outTTname, FONTDIR);
	strcat( outTTname, inTTname);

	printf( "i = %s\no = %s\n", inTTname, outTTname);

	// read the stuff

	FILE* fp = fopen( inTTname, "rb");
	if( !fp) {
		printf( "Cannot read \"%s\"\n", inTTname);
		return -1;
	}

	struct stat st;
	stat( inTTname, &st);
	int flen = st.st_size;
	U8* buf = new U8[ flen + 3];
	for( int ibuf = 0; ibuf < 3; ++ibuf) buf[ flen + ibuf] = 0;
	printf( "TTFsize = %d\n", flen);

	fread( buf, 1, flen, fp);
	fclose( fp);

	// patch as requested

	for( int i = 2; i < argc; i += 3) {
		char* p = argv[ i];
		if( p[0] != '-' || p[1] != 'p')
			return -i;

		int addr = 0;
		p = argv[ i+1];
		if( p[0] != '0' || p[1] != 'x')
			return -i;
		for( p += 2; *p; ++p) {
			char c = *p | 0x20;
			addr = (addr << 4) + ((c>'9') ? c-'a'+10 : c-'0');
		}
		if( addr >= flen)
			return -i;

		p = argv[ i+2];
		if( p[0] != '0' || p[1] != 'x')
			return -i;
		for( p += 2; *p; ++p, ++addr) {
			int c = *p | 0x20;
			U8 nhex = (c>'9') ? c-'a'+10 : c-'0';
			c = *++p | 0x20;
			nhex <<= 4;
			nhex |= (c>'9') ? c-'a'+10 : c-'0';

			printf( "patch %05X = %02X -> %02X\n",
				addr, buf[addr], nhex);

			buf[ addr] = nhex;
		}
	}

	// update the checksums

	int nTables = (buf[4] << 8) + buf[5];
	//printf( "nTables = %d\n", nTables);
	U8* headTable = 0;
	for( int iTable = 0; iTable < nTables; ++iTable) {
		U8* b = &buf[ 12 + iTable * 16];
		int name = (b[0]<<24) + (b[1]<<16) + (b[2]<<8) + b[3];
		int offset = (b[8]<<24) + (b[9]<<16) + (b[10]<<8) + b[11];
		int length = (b[12]<<24) + (b[13]<<16) + (b[14]<<8) + b[15];
		//printf( "offset = %08X, length = %08X\n", offset, length);
		int check = checksum( buf + offset, length);
		b[4] = (U8) (check >> 24);
		b[5] = (U8) (check >> 16);
		b[6] = (U8) (check >> 8);
		b[7] = (U8) check;
		//printf( "checksum[%d] = %08X\n", iTable, check);
		if( name == 0x68656164) {
			headTable = buf + offset;
			//printf( "headOffset = %08X\n", offset);
		}
	}

	int check = checksum( buf, flen) - 0xB1B0AFBA;
	//printf( "csAdjust = %08X\n", check);
	headTable[8] = (U8) (check >> 24);
	headTable[9] = (U8) (check >> 16);
	headTable[10] = (U8) (check >> 8);
	headTable[11] = (U8) check;

	// write the patched file

	fp = fopen( outTTname, "wb");
	if( !fp) {
		printf( "Cannot write \"%s\"\n", outTTname);
		return -1;
	}
	fwrite( buf, 1, flen, fp);
	fclose( fp);

	delete[] buf;

	return 0;
}

#ifndef WIN32
int main( int argc, char** argv)
{
	if( argc < 4) {
		usage();
		return 0;
	}

	int err = patchttf( argc, argv);
	if( err)
		printf( "Error at arg %d\n", -err);

	return err;
}
#endif


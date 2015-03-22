// Utilities for efficient access to the TTFfile
// (C) Copyright 1997-1998 Herbert Duerr

/* [jer] = Joshua Rodd, <jerodd@usa.net> (OS/2 porter) */
/* I made these utilities inefficient. =) */

#define DEBUG 1
#include "ttf.h"

#ifdef WIN32
        #include <windows.h>
        #include <io.h>
#else
        #include <fcntl.h>
#ifdef __EMX__
        #include <stdlib.h>
        #include <sys/types.h>
        #define INCL_NOPMAPI
        #define INCL_DOSMEMMGR
        #define INCL_NOCOMMON
        #include <os2.h>
#else
        #include <sys/mman.h>
#endif
        #include <sys/stat.h>
        #include <unistd.h>
#endif

#include <stddef.h>
#include <stdlib.h>

void* allocMem( int size)
{
        void* ptr;
#ifdef MAP_ANON
        ptr = mmap( 0, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON, -1, 0);
        if( ptr == (void*)-1)
                ptr = 0;
#elif defined(WIN32)
        ptr = VirtualAlloc( NULL, size,
                        MEM_COMMIT, PAGE_READWRITE);
#else
        ptr = malloc( size);
#endif
        return ptr;
}

void* shrinkMem( void* ptr, int oldsize, int newsize)
{
#if defined(MAP_ANONYMOUS) && !defined(__osf__)
        if( oldsize > newsize)
                ptr = mremap( ptr, oldsize, newsize, 0);
        if( ptr == (void*)-1)
                ptr = 0;
#endif
        return ptr;
}

void deallocMem( void* ptr, int size)
{
#ifdef MAP_ANON
        munmap( ptr, size);
#elif defined(WIN32)
        VirtualFree( ptr, size, MEM_DECOMMIT);
#else
        free( ptr);
#endif
}

#ifdef MEMDEBUG

#define MEMPTRS 8192
typedef struct { void* ptr; int len;} memstruct;
static memstruct memdbg[ MEMPTRS];
static int memidx = 0;
static int memcount = 0;
static int memused = 0;

void cleanupMem()
{
        dprintf0( "Memory holes:\n");
        for( int i = 0; i < memidx; ++i)
                if( memdbg[ i].ptr)
                        dprintf2( "MEM hole[%3d] = %p\n", i, memdbg[ i].ptr);
        if( memcount != 0)
                dprintf1( "MEM hole: memcount = %d\n", memcount);
}

void* operator new[]( size_t size)
{
        void* ptr = malloc( size);
        memused += size;

        dprintf2( "MEM new[]( %5d) = %p", size, ptr);
        dprintf2( ", memcount = %d, memidx = %d", ++memcount, memidx);

        int i = memidx;
        while( --i >= 0 && memdbg[ i].ptr);
        if( i <= 0) i = memidx++;
        dprintf2( ", idx = %d, used %d\n", i, memused);

        memdbg[ i].ptr = ptr;
        memdbg[ i].len = size;
        return ptr;
}

void operator delete[]( void* ptr)
{
        dprintf1( "MEM delete[]( %p)", ptr);
        dprintf2( ", memcount = %d, memidx = %d\n", --memcount, memidx);

        int i = memidx;
        while( --i >= 0 && memdbg[ i].ptr != ptr);
        if( i >= 0) {
                memdbg[ i].ptr = 0;
                memused -= memdbg[ i].len;
                dprintf2( ", idx = %d, used %d\n", i, memused);
                if( ++i == memidx) --memidx;
        } else
                dprintf0( "Cannot delete!\n");

        free( ptr);
}

void* operator new( size_t size)
{
        void* ptr = malloc( size);
        memused += size;

        dprintf2( "MEM new( %7d) = %p", size, ptr);
        dprintf2( ", memcount = %d, memidx = %d", ++memcount, memidx);

        int i = memidx;
        while( --i >= 0 && memdbg[ i].ptr);
        if( i <= 0) i = memidx++;
        dprintf2( ", idx = %d, used %d\n", i, memused);

        memdbg[ i].ptr = ptr;
        memdbg[ i].len = size;
        return ptr;
}

void operator delete( void* ptr)
{
        dprintf1( "MEM delete( %p)", ptr);
        dprintf2( ", memcount = %d, memidx = %d", --memcount, memidx);

        int i = memidx;
        while( --i >= 0 && memdbg[ i].ptr != ptr);
        if( i >= 0) {
                memdbg[ i].ptr = 0;
                memused -= memdbg[ i].len;
                dprintf2( ", idx = %d, used %d\n", i, memused);
                if( ++i == memidx) --memidx;
        }
        else
                dprintf0( "Cannot delete!\n");

        free( ptr);
}

#endif /* MEMDEBUG */


RandomAccessFile::RandomAccessFile( char* fileName)
{
#ifdef WIN32
        void* fd = CreateFile( fileName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if( fd == INVALID_HANDLE_VALUE) {
                fprintf( stderr, "Cannot open \"%s\"\n", fileName);
                exit( -1);
        }
        length = GetFileSize( fd, NULL);
        HANDLE hdl = CreateFileMapping( fd, NULL, PAGE_READONLY, 0, length, "TTF mapping");
        base = (byte*) MapViewOfFile( hdl, FILE_MAP_READ, 0, 0, length);
        CloseHandle( fd);
#elif defined(__EMX__)
        /* This added by [jer] for OS/2. It's ugly: it dumps the entire file */
        /* into memory. This is why xfsttos2 is a memory hog. */
        int fd = open( fileName, O_RDONLY);
        int readen;
        struct stat st;
        if( fd <= 0) {
                fprintf( stderr, "Cannot open \"%s\"\n", fileName);
                ptr = base = 0;
                return;
        }
        fstat( fd, &st);
        length = st.st_size;
        fprintf(stderr,"loading `%s'...",fileName);
/*      base=(unsigned char*)malloc(length);if(!base){
         fprintf(stderr," out of memory\n");ptr=base=0;return;}*/
        /* Don't use malloc(); use DosAllocMem so we can be a memory pig. */
        {void*sel;void**psel=&sel;
         APIRET rc=DosAllocMem(psel,length,PAG_COMMIT|PAG_READ|PAG_WRITE);
         if(rc){
          if(rc==8)fprintf(stderr," out of memory\n");else
           fprintf(stderr," memory error\n"\
            "For more information, run the command `help %lu'.\n",rc);
          ptr=base=0;return;
         }
         base=(unsigned char*)sel;
        }
        readen=read(fd,base,length);if(!readen){
         perror(" file appears empty");ptr=base=0;return;}
        if(readen<0){
         perror(" error loading");ptr=base=0;return;}
        if(readen<length){
         perror(" file truncated");ptr=base=0;return;}
        fprintf(stderr,"loaded.\n");
        close( fd);
#else
        int fd = open( fileName, O_RDONLY);
        if( fd <= 0) {
                fprintf( stderr, "Cannot open \"%s\"\n", fileName);
                ptr = base = 0;
                return;
        }
        struct stat st;
        fstat( fd, &st);
        length = st.st_size;
        base = (U8*) mmap( 0L, length, PROT_READ, MAP_SHARED, fd, 0L);
        close( fd);
#endif
        ptr = absbase = base;
}

void RandomAccessFile::closeRAFile()
{
        if( absbase && absbase == base && length > 0) {
#ifdef WIN32
                UnmapViewOfFile( base);
#elif defined(__EMX__)
/*              free(base);*/
                DosFreeMem(base);
#else
                munmap( base, length);
#endif
        }
}

U32 RandomAccessFile::calcChecksum()
{
        U32 checksum = 0;
        U8* saveptr = ptr;
        for( int len = length >> 2; --len >= 0;)
                checksum += readUInt();
        if( length & 3)
                checksum += readUInt() & (-1 << ((-length & 3) << 3));
        ptr = saveptr;
        dprintf1( "checksum is %08X\n", calcChecksum());
        return checksum;
}


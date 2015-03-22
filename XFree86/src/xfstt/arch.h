// architecture specifics, little endian 32bit
// (C) Copyright 1997-1998 Herbert Duerr

// environment specific

#if defined(__i386__) || defined(__EMX__)
        #define MSB_BYTE_FIRST  0       // MSB=1, LSB=0
        #define MSB_BIT_FIRST   0       // MSB=2, LSB=0
        #define FSBYTEORDER     'l'     // 'l' little endian, 'B' big endian
#ifndef __EMX__
        #define LOGSLP          5       // default SCANLINEPAD is 32bits = 1<<5
                                        // change it to 3 for some font clients
#else
        #define LOGSLP 3
#endif
#else
        #define MSB_BYTE_FIRST  1
        #define MSB_BIT_FIRST   2
        #define FSBYTEORDER     'B'
        #define LOGSLP          5
#endif

// architecture specific

typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned int    U32;

typedef signed char     S8;
typedef signed short    S16;
typedef signed int      S32;

// byte swapping (doing it by ntohl/ntohs works only on little endian CPUs)
#define bswapl(x)       ntohl(x)
#define bswaps(x)       ntohs(x)

#ifdef __i386__
        // MULDIV is worth putting in assembler,
        // undefine it to use the generic method
        #define MULDIV muldiv
        inline int muldiv( int a, int b, int c)
        {
                int r;
                // 32Bit * 32Bit / 32Bit with intermediate 64Bit divisor
                __asm__( "imull %2\n\tidiv %3,%0" :
                         "=a"(r) : "0"(a), "d"(b), "g"(c));
                return r;
        }
#endif


#ifndef MULDIV
        // 64bit types are only needed for temporary MULDIV results
        #ifndef WIN32
                typedef long long               S64;
                typedef unsigned long long      U64;
        #else
                typedef __int64                 S64;
                typedef unsigned __int64        U64;
        #endif
#endif

//=========== add special cases here ==================

#ifdef WIN32
        typedef void* __ptr_t;
        #define setbuffer(x1,x2,x3)
        #include <stdlib.h>
        extern FILE* outfile;
#else
        #define outfile stdout
#endif

#ifdef __sgi
        #undef LOGSLP
        #define LOGSLP 3
#endif

#ifdef __EMX__
#define _WITH_UNDERSCORE
#define setsid _setsid
#endif

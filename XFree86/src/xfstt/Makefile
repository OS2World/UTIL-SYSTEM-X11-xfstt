# @(#)Makefile.xfsttos2 0.9.10

# This is geared for emx+gcc and g++ (Ver. 2.7.2.x) with XFree86 for OS/2.
# @(#)Makefile.xfsttos2 0.9.10 1998/12/22

# Optimization and support for x11make.exe(gnumake)/LINK386.EXE
# in 1999/12 added by
#               Arnd.H.Hanses@rz.ruhr-uni-bochum.de
# Please mail bugs, hints and fixes concerning these changes to me.
# Note: For use with gdb you must use the original Makefile.


# experimental optimizing for pentium (i586). Change or comment out if necessary.
OPTIMIZE = -O2 -m486 -Zcrtdll -malign-loops=2 -ffast-math \
        -malign-jumps=2 -malign-functions=2 -fno-strength-reduce

# experimental optimizing for i486DX. Change or comment out if necessary.
#OPTIMIZE = -O2 -m486 -Zcrtdll -ffast-math -fno-strength-reduce

# experimental optimizing for i386/387. Change or comment out if necessary.
#OPTIMIZE = -O2 -Zcrtdll -ffast-math -fno-strength-reduce

# experimental optimizing for generic x86. Change or comment out if necessary.
#OPTIMIZE = -O2 -Zcrtdll -fno-strength-reduce




SHELL    = d:/bin/sh.exe        # you need an unix-like shell and x11make.exe(gnumake)
RM     = rm -f

O      = .obj

# for info only
EXEEXT = .exe
A      =.lib
AR     = emxomfar

prefix = $(X11ROOT)/Xfree86

# where the installed binary goes
bindir = $(prefix)/bin


# use these with OS/2 EMX to link with LINK386.EXE (tested with EMX 0.9c)
# increase or decrease stack size in -Zstack if necessary
CC      = gcc -DOS2 -Zomf -Zbin-files

CFLAGS = -Wall -pedantic -DMAGNIFY=0 $(OPTIMIZE)

LFLAGS = -Zcrtdll -Zomf -Zstack 1500 -s -L$(X11ROOT)/XFree86/lib -lwrap

OBJS =  RAFile$(O) TTFont$(O) NameTable$(O) HeadTable$(O) MaxpTable$(O) CmapTable$(O) \
        LocaTable$(O) GlyfTable$(O) LtshTable$(O) HdmxTable$(O) VdmxTable$(O) FpgmTable$(O) \
        PrepTable$(O) GaspTable$(O) HheaTable$(O) HmtxTable$(O) CvtTable$(O) Os2Table$(O) \
        KernTable$(O) EbdtTable$(O) EblcTable$(O) RasterHints$(O) RasterScale$(O) RasterDraw$(O)


.SUFFIXES: .cpp $(O) .s

.cpp$(O):
        $(CC) $(CFLAGS) -c $<

.cpp.s:
        $(CC) -S $(CFLAGS) -c $<


all: xfstt.exe xfstt.gz

install: xfstt.exe xfstt.gz

xfstt.exe: $(OBJS) xfstt$(O) encoding$(O) $(DEPHACK)
        $(CC) -o $@ $(OBJS) xfstt$(O) encoding$(O) $(LFLAGS) -lm -lsocket

install: xfstt.exe xfstt.gz
        cp xfstt.exe $(bindir) >nul
        cp xfstt.gz  $(prefix)/XFree86/man1>nul
        -mkdir $(X11ROOT)/XFree86/man 2>nul
        -mkdir $(X11ROOT)/XFree86/man1 2>nul
        -mkdir $(X11ROOT)/XFree86/lib/X11/fonts/TrueType 2>nul
        echo.
        echo Remember to copy fonts from \WINDOWS\SYSTEM or \OS2\MDOS\WINOS2\SYSTEM
        echo to $(X11ROOT)\XFree86\lib\fonts\TrueType. You should copy all TTF files; i.e.:
        echo     copy \WINDOWS\SYSTEM\*.TTF $(X11ROOT)\XFree86\lib\X11\fonts\TrueType
        echo     copy \OS2\MDOS\WINOS2\SYSTEM\*.TTF $(X11ROOT)\XFree86\X11\lib\fonts\TrueType
        echo (This requires that Windows or Win-OS/2 be installed.)
        echo Of course, you can use fonts from other sources than Windows or Win-OS/2.
        echo After you populate $(X11ROOT)\XFree86\lib\X11\fonts\TrueType with fonts,
        echo run this command:
        echo     xfstt --sync
        echo This will prepare the fonts for use. See README.OS2 for more information.

perftest.exe: $(OBJS) perftest$(O) $(DEPHACK)
        $(CC) -o $@ $(OBJS) perftest$(O) $(LFLAGS) -lm

patchttf.exe: patchttf.cpp $(DEPHACK)
        $(CC) $(CFLAGS) -o $@ $*.cpp

xfstt$(O): xfstt.cpp xfstt.h $(DEPHACK)
        $(CC) $(CFLAGS) -c $*.cpp -I$(X11ROOT)/XFree86/include/X11/fonts -I$(X11ROOT)/XFree86/include/X11 -I$(X11ROOT)/XFree86/include

encoding$(O): encoding.cpp encoding.h $(DEPHACK)
        $(CC) $(CFLAGS) -c $*.cpp

showttf.exe: $(OBJS) showttf$(O) $(DEPHACK)
        $(CC) $(CFLAGS) -o $@ $(OBJS) showttf$(O) $(LFLAGS) -lX11 -lm -Zmtd -Z__ST_ERRNO__

xfstt.gz: xfstt.1
        gzip <$**>$@


clean:
        -$(RM) $(PROG) a.out core *$(O) *.obj *.omf xfstt.exe showttf.exe perftest.exe \
        patchttf.exe 2>nul


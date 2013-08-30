# $Id: makefile,v 1.2 2005/02/14 17:33:24 glass Exp $

CFLAGS=-Gm -Q+ -W3 -Wuse+ -O+ -Gl+ -Ge-
LFLAGS=/B"/DLL /OPTF /PM:PM /EXEP:2 /MAP"

!ifdef DEBUG
CFLAGS= $(CFLAGS) -Ti+
!endif

.cpp.obj:
    $(CC) $(CFLAGS) /c /Fo$@ $< 

all: ..\wxtask.dll wxtask.cpp wxtask.h 

..\wxtask.dll: wxtask.obj wxtask.res
    $(CC) $(CFLAGS) $(LFLAGS) /Fe$@ @<< 
    wxtask.obj
<<
    rc -n wxtask.res $*.dll

wxtask.res: wxtask.rc wxtask.h
    rc -n -r $*.rc $*.res

clean:
   @-erase *.o*
   @-erase *.res
   @-erase *.map

#                  RANDOMAC.MAKE

# Author:        Agner Fog
# Date created:  2001
# Last modified: 2010-08-02
# Project:       randoma.zip, randomc.zip, stocc.zip
# Source URL:    www.agner.org/random
# Description:
#
# Makefile for randoma function library and randomc and stocc class libraries.
# See ran-instructions.pdf for instructions

# The following tools are required for building this library package:
# Microsoft nmake or other make utility
# Microsoft assembler ml.exe and ml64.exe
# Object file converter objconv.exe (www.agner.org/optimize)
# Winzip command line version (www.winzip.com) or other zip utility

# Copyright 2001-2010 by Agner Fog. 
# GNU General Public License http://www.gnu.org/licenses/gpl.html


# Main targets:
all: randomc.zip stocc.zip randoma.zip

# Make randomc.zip
randomc.zip: ran-instructions.pdf randomc.h sfmt.h \
mother.cpp mersenne.cpp sfmt.cpp rancombi.cpp userintf.cpp \
ex-ran.cpp testirandomx.cpp \
license.txt readme.txt
  wzzip $@ $?

  
# Make stocc.zip
stocc.zip: \
ran-instructions.pdf distrib.pdf sampmet.pdf \
stocc.h randomc.h \
stoc1.cpp stoc2.cpp stoc3.cpp \
wnchyppr.cpp fnchyppr.cpp erfres.cpp erfresmk.cpp \
ex-stoc.cpp ex-cards.cpp ex-lotto.cpp \
ex-evol1.cpp ex-evol2.cpp \
testpois.cpp testbino.cpp testhype.cpp \
testfnch.cpp testwnch.cpp testmfnc.cpp testmwnc.cpp \
license.txt readme.txt
  wzzip $@ $?


# Make randoma.zip
randoma.zip: \
ran-instructions.pdf license.txt readme.txt \
randoma.h randomc.h testrandomac.cpp \
randomacof32.lib randomacof64.lib \
randomaomf32.lib  \
randomaelf32.a randomaelf32p.a randomaelf64.a \
randomamac32.a randomamac32p.a randomamac64.a  \
randomad32.dll randomad32.lib randomad64.dll randomad64.lib \
randomasrc.zip
  wzzip $@ $?

  
# Make zip archive of source code  
randomasrc.zip: \
MakeRandomac.bat randomac.make randoma.h randomah.asi \
instrset32.asm instrset64.asm \
rdtsc32.asm rdtsc64.asm \
physseed32.asm physseed64.asm \
mersenne32.asm mersenne64.asm \
mother32.asm mother64.asm \
sfmt32.asm sfmt64.asm \
randomad32.asm randomad32.def randomad32.exp \
randomad64.asm randomad64.def randomad64.exp
  wzzip $@ $?


# Build 32 bit static libraries:

# 32 bit COFF library
randomacof32.lib: \
instrset32.obj32 rdtsc32.obj32  \
mersenne32.obj32 mother32.obj32 sfmt32.obj32 physseed32.obj32
  objconv -fcof32 -wex -lib $@ $?

# 32 bit ELF library, position dependent
randomaelf32.a: \
instrset32.o32 rdtsc32.o32 physseed32.o32 \
mersenne32.o32 mother32.o32 sfmt32.o32
  objconv -felf32 -nu -wex -lib $@ $?

# 32 bit ELF library, position independent
randomaelf32p.a: \
instrset32.o32pic rdtsc32.o32pic physseed32.o32pic \
mersenne32.o32pic mother32.o32pic sfmt32.o32pic
  objconv -felf32 -nu -wex -lib $@ $?

# 32 bit OMF library
randomaomf32.lib: randomacof32.lib  
  objconv -fomf32 -nu -wex $** $@
  
# 32 bit Mach-O library, position dependent
randomamac32.a: randomaelf32.a
  objconv -fmac32 -nu -wex -wd1050 $** $@
  
# 32 bit Mach-O library, position independent
randomamac32p.a: randomaelf32p.a
  objconv -fmac32 -nu -wex $** $@
  

# Build 64 bit static libraries:

# 64 bit COFF library Windows
randomacof64.lib: \
instrset64.obj64 rdtsc64.obj64 physseed64.obj64 \
mersenne64.obj64 mother64.obj64 sfmt64.obj64
  objconv -fcof64 -wex -lib $@ $?

# 64 bit ELF library Unix
randomaelf64.a: \
instrset64.o64 rdtsc64.o64 physseed64.o64 \
mersenne64.o64 mother64.o64 sfmt64.o64
  objconv -felf64 -nu -wex -wd1029 -lib $@ $?

# 64 bit Mach-O library
randomamac64.a: randomaelf64.a
  objconv -fmac64 -nu -wex $** $@
  

# Build 32 bit and 64 bit dybamic libraries:

# Convert 32 bit COFF library to DLL
randomad32.dll: randomacof32.lib randomad32.def randomad32.obj32
  link /DLL /DEF:randomad32.def /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:DllEntry randomad32.obj32 randomacof32.lib

# Convert 64 bit COFF library to DLL
randomad64.dll: randomacof64.lib randomad64.def randomad64.obj64
  link /DLL /DEF:randomad64.def /SUBSYSTEM:WINDOWS /ENTRY:DllEntry randomad64.obj64 randomacof64.lib


# Dependences on header files
sfmt32.asm: randomah.asi


# Generic rules for assembling

# Generic rule for assembling 32-bit code for Windows (position dependent)
.asm.obj32:
  ML /c /Cx /W3 /coff /DWINDOWS /Fo$*.obj32 $<

# If listing and debug information desired:
# ML /c /Cx /W3 /coff /DWINDOWS /Fl /Zd /Fo$*.obj32 $<

# Generic rule for assembling 32-bit for Unix, position-dependent
.asm.o32:
  ML /c /Cx /W3 /coff /DUNIX /Fo$*.o32 $<
  objconv -felf32 -nu -wd2005 $@ $@
 
# Generic rule for assembling 32-bit for Unix, position-independent
.asm.o32pic:
  ML /c /Cx /W3 /coff /DUNIX /DPOSITIONINDEPENDENT /Fo$*.o32pic $<
  objconv -felf32 -wd2005 -nu $@ $@
  
# Generic rule for assembling 64-bit code for Windows
.asm.obj64:
  ML64 /c /Cx /W3 /DWINDOWS /Fo$*.obj64 $<

# Generic rule for assembling 64-bit code for Linux, BSD, Mac
.asm.o64:
  ML64 /c /Cx /W3 /DUNIX /Fo$*.o64 $<
  objconv -felf64 -wd2005 -nu $@ $@

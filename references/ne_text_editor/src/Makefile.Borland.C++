.AUTODEPEND

#		*Translator Definitions*
CC = bcc +NE.CFG
TASM = TASM
TLIB = tlib
TLINK = tlink
LIBPATH = F:\BJH\BC\LIB
INCLUDEPATH = F:\BJH\BC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 debug.obj \
 ecmdarg.obj \
 ecmdcomp.obj \
 ecmdsub.obj \
 ecompr.obj \
 ecrash.obj \
 ecutcopy.obj \
 edisplay.obj \
 ee1.obj \
 ee2.obj \
 ee3.obj \
 ee4.obj \
 eerror.obj \
 efile.obj \
 eglobals.obj \
 einit.obj \
 ekey.obj \
 ekeysub.obj \
 eline.obj \
 elocal.obj \
 ematch.obj \
 ematchr.obj \
 erdseqs.obj \
 escrnrdl.obj \
 escrnsub.obj \
 estore.obj \
 eversion.obj \
 rdargs.obj \
 scommon.obj \
 syspc.obj

#		*Explicit Rules*
ne.exe: ne.cfg $(EXE_dependencies)
  $(TLINK) /v/s/c/P-/L$(LIBPATH) @&&|
c0h.obj+
debug.obj+
ecmdarg.obj+
ecmdcomp.obj+
ecmdsub.obj+
ecompr.obj+
ecrash.obj+
ecutcopy.obj+
edisplay.obj+
ee1.obj+
ee2.obj+
ee3.obj+
ee4.obj+
eerror.obj+
efile.obj+
eglobals.obj+
einit.obj+
ekey.obj+
ekeysub.obj+
eline.obj+
elocal.obj+
ematch.obj+
ematchr.obj+
erdseqs.obj+
escrnrdl.obj+
escrnsub.obj+
estore.obj+
eversion.obj+
rdargs.obj+
scommon.obj+
syspc.obj
ne,ne
emu.lib+
mathh.lib+
ch.lib
|


#		*Individual File Dependencies*
debug.obj: ne.cfg debug.c 

ecmdarg.obj: ne.cfg ecmdarg.c 

ecmdcomp.obj: ne.cfg ecmdcomp.c 

ecmdsub.obj: ne.cfg ecmdsub.c 

ecompr.obj: ne.cfg ecompr.c 

ecrash.obj: ne.cfg ecrash.c 

ecutcopy.obj: ne.cfg ecutcopy.c 

edisplay.obj: ne.cfg edisplay.c 

ee1.obj: ne.cfg ee1.c 

ee2.obj: ne.cfg ee2.c 

ee3.obj: ne.cfg ee3.c 

ee4.obj: ne.cfg ee4.c 

eerror.obj: ne.cfg eerror.c 

efile.obj: ne.cfg efile.c 

eglobals.obj: ne.cfg eglobals.c 

einit.obj: ne.cfg einit.c 

ekey.obj: ne.cfg ekey.c 

ekeysub.obj: ne.cfg ekeysub.c 

eline.obj: ne.cfg eline.c 

elocal.obj: ne.cfg elocal.c 

ematch.obj: ne.cfg ematch.c 

ematchr.obj: ne.cfg ematchr.c 

erdseqs.obj: ne.cfg erdseqs.c 

escrnrdl.obj: ne.cfg escrnrdl.c 

escrnsub.obj: ne.cfg escrnsub.c 

estore.obj: ne.cfg estore.c 

eversion.obj: ne.cfg eversion.c 

rdargs.obj: ne.cfg rdargs.c 

scommon.obj: ne.cfg scommon.c 

syspc.obj: ne.cfg syspc.c 

#		*Compiler Configuration File*
ne.cfg: ne.mak
  copy &&|
-R
-mh
-K
-w+
-g255
-N
-v
-y
-G
-O
-Og
-Oe
-Ov
-Ol
-Ob
-Oi
-Z
-h
-vi-
-Fc
-I$(INCLUDEPATH)
-L$(LIBPATH)
-DDebug=1
-P-.C
-Ff
| ne.cfg



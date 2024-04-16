# Top level makefile for the NE editor. This level just checks for
# a system-dependent makefile, and calls it if present. It also has
# targets for creating the system-dependent file by linking.

# Set TARGET = nenew to make a version with Perl-compatible regular
# expression handling (only). This can be done dynamically by obeying
# "make TARGET=nenewp" or statically by editing here.

TARGET = nenew

ne:		OS_Make ne_sys

clean:;		/bin/rm -f *.o

ne_sys:;	make -f OS_Make TARGET=$(TARGET)

OS_Make:;	@echo '*** No OS_Make file has been set up.'
		@echo '*** Type one of the following to specify your system:'
		@echo ' '
		@echo '   make BSDI_gcc'
		@echo '   make DEC_OSF1'
		@echo '   make HP-UX_cc    (uses the cc compiler)'
		@echo '   make HP-UX_gcc   (uses the gcc compiler)'
		@echo '   make IRIX4'
		@echo '   make IRIX5'
		@echo '   make FreeBSD'
		@echo '   make Linux_cc    (uses the cc compiler)'
		@echo '   make Linux_gcc   (uses the gcc compiler)'
		@echo '   make NetBSD'
		@echo '   make SunOS4_acc  (uses the acc compiler)'
		@echo '   make SunOS4_gcc  (uses the gcc compiler)'
		@echo '   make SunOS5_cc   (uses the cc compiler)'
		@echo '   make SunOS5_gcc  (uses the gcc compiler)'
		@echo '   make Ultrix_mips (uses the gcc compiler)'
		@echo ''
		@exit 1

DEC_OSF1:       clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_DEC_OSF1 OS_Make
		ln -s elocal.osf1.h elocal.h
		ln -s elocal.osf1.c elocal.c
		ln -s osf1local.h unixlocal.h
		@echo '*** Now run make again'

HP-UX_cc:       clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_HP-UX.cc OS_Make
		ln -s elocal.hpux.h elocal.h
		ln -s elocal.hpux.c elocal.c
		ln -s hpuxlocal.h unixlocal.h
		@echo '*** Now run make again'

HP-UX_gcc:      clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_HP-UX.gcc OS_Make
		ln -s elocal.hpux.h elocal.h
		ln -s elocal.hpux.c elocal.c
		ln -s hpuxlocal.h unixlocal.h
		@echo '*** Now run make again'

IRIX4:          clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_IRIX4 OS_Make
		ln -s elocal.irix5.h elocal.h
		ln -s elocal.irix5.c elocal.c
		ln -s irix4local.h unixlocal.h
		@echo '*** Now run make again'

IRIX5:          clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_IRIX5 OS_Make
		ln -s elocal.irix5.h elocal.h
		ln -s elocal.irix5.c elocal.c
		ln -s irix5local.h unixlocal.h
		@echo '*** Now run make again'

Linux_cc:       clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_Linux.cc OS_Make
		ln -s elocal.linux.h elocal.h
		ln -s elocal.linux.c elocal.c
		ln -s linuxlocal.h unixlocal.h
		@echo '*** Now run make again'

Linux_gcc:      clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_Linux.gcc OS_Make
		ln -s elocal.linux.h elocal.h
		ln -s elocal.linux.c elocal.c
		ln -s linuxlocal.h unixlocal.h
		@echo '*** Now run make again'

BSDI_gcc:       clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_BSDI.gcc OS_Make
		ln -s elocal.bsdi.h elocal.h
		ln -s elocal.bsdi.c elocal.c
		ln -s bsdilocal.h unixlocal.h
		@echo '*** Now run make again'

FreeBSD:        clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_FreeBSD OS_Make
		ln -s elocal.freebsd.h elocal.h
		ln -s elocal.freebsd.c elocal.c
		ln -s freebsdlocal.h unixlocal.h
		@echo '*** Now run make again'

NetBSD:         clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_NetBSD OS_Make
		ln -s elocal.netbsd.h elocal.h
		ln -s elocal.netbsd.c elocal.c
		ln -s netbsdlocal.h unixlocal.h
		@echo '*** Now run make again'

Ultrix_mips:    clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_Ultrix_mips OS_Make
		ln -s elocal.ultrix.h elocal.h
		ln -s elocal.ultrix.c elocal.c
		ln -s ultrixlocal.h unixlocal.h
		@echo '*** Now run make again'

SunOS4_acc:     clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_SunOS4.acc OS_Make
		ln -s elocal.sunos4.h elocal.h
		ln -s elocal.sunos4.c elocal.c
		ln -s sunos4local.h unixlocal.h
		@echo '*** Now run make again'

SunOS4_gcc:     clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_SunOS4.gcc OS_Make
		ln -s elocal.sunos4.h elocal.h
		ln -s elocal.sunos4.c elocal.c
		ln -s sunos4local.h unixlocal.h
		@echo '*** Now run make again'

SunOS5_cc:      clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_SunOS5.cc OS_Make
		ln -s elocal.sunos5.h elocal.h
		ln -s elocal.sunos5.c elocal.c
		ln -s sunos5local.h unixlocal.h
		@echo '*** Now run make again'

SunOS5_gcc:     clean
		/bin/rm -f OS_Make elocal.h elocal.c unixlocal.h
		ln -s OS_Make_SunOS5.gcc OS_Make
		ln -s elocal.sunos5.h elocal.h
		ln -s elocal.sunos5.c elocal.c
		ln -s sunos5local.h unixlocal.h
		@echo '*** Now run make again'

# End

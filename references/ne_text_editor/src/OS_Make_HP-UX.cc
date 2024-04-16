# System-dependent Unix makefile for the NE editor.
# Version for HP-UX.

FLAGS   = CC=cc \
          CFLAGS="-Dunixwinsz -DHAVE_TERMCAP -DNO_VDISCARD \
            -D_HPUX_SOURCE -Aa" \
          LIB_TERMINAL=-ltermcap 
        
newne:;   @make -f BaseMake $(FLAGS) $(TARGET)

# End

# System-dependent Unix makefile for the NE editor.
# Version for SunOS5 using Sun's cc compiler.

FLAGS   = CC=cc \
          CFLAGS="-O -Dunixwinsz" \
          LIB_TERMINAL=-lcurses 
        
newne:;   @make -f BaseMake $(FLAGS) $(TARGET)

# End

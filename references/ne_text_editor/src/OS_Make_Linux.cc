# System-dependent Unix makefile for the NE editor.
# Version for Linux using the cc compiler.

#FLAGS   = CC=cc \
#          CFLAGS="-Dunixwinsz -DHAVE_TERMCAP -O" \
#          LIB_TERMINAL=-ltermcap 

FLAGS   = CC=cc \
          CFLAGS="-Dunixwinsz -O" \
          LIB_TERMINAL=-lcurses 
        
newne:;   @make -f BaseMake $(FLAGS) $(TARGET)

# End

# Project: cobgdb
# Makefile originally  created by Dev-C++ 5.11

# detect if running under unix by finding 'rm' in $PATH :
#ifeq ($(wildcard $(addsuffix /rm,$(subst :, ,$(PATH)))),)
ifeq ($(OS),Windows_NT)
    WINMODE = 1
else
    ifeq ($(wildcard $(addsuffix /rm,$(subst :, ,$(PATH)))),)
        WINMODE = 1
    else
        WINMODE = 0
    endif
endif

# path where the source resides - same as current Makefile's directory
SRCDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# compat only, could be dropped otherwise
CPPFLAGS += $(INCS)

OBJ      = cobgdb.o terminal.o read_file.o parser.o parser_mi2.o gdb_process.o mi2.o testMI2.o testParser.o variables.o debugger.o output.o highlight.o string_parser.o

#
# Windows
# objdump -x cobgdb.exe | findstr /R /C:"DLL"
#
ifeq ($(WINMODE),1)
  CC       = gcc.exe
  RES      = 
  COMP     = comp.bat
  OBJ     += realpath.o
  BIN      = cobgdb.exe
  RM       = del
  CP       = copy

else
#
# Linux
#

  CC       = gcc
  RES      =
  COMP     = comp.sh
  BIN      = cobgdb
  RM       = rm -f
  CP       = cp

  # Check whether the file Xlib.h exists in /usr/include/X11/Xlib.h (or in a similar include path, if different on your system).
  X11_HEADER_EXISTS := $(shell [ -f /usr/include/X11/Xlib.h ] && echo yes || echo no)
  ifeq ($(X11_HEADER_EXISTS),yes)
    LIBS     += -lX11
    CPPFLAGS += -DHAVE_X11 -I/usr/include/X11
  endif
endif

CFLAGS   = $(CPPFLAGS) -fdiagnostics-color=always -g -Wall -Wextra

.PHONY: all all-before all-after clean clean-custom copy

all: all-before $(BIN) all-after

ifeq ($(WINMODE),1)
copy:
	$(CP) $(BIN) windows
	$(CP) $(COMP) windows
	$(RM) $(BIN)
endif

clean: clean-custom
	${RM} $(OBJ)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LIBS)

cobgdb.o: $(SRCDIR)/cobgdb.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/cobgdb.c -o cobgdb.o $(CFLAGS)

terminal.o: $(SRCDIR)/terminal.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/terminal.c -o terminal.o $(CFLAGS)

read_file.o: $(SRCDIR)/read_file.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/read_file.c -o read_file.o $(CFLAGS)

parser.o: $(SRCDIR)/parser.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/parser.c -o parser.o $(CFLAGS)

gdb_process.o: $(SRCDIR)/gdb_process.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/gdb_process.c -o gdb_process.o $(CFLAGS)

mi2.o: $(SRCDIR)/mi2.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/mi2.c -o mi2.o $(CFLAGS)

parser_mi2.o: $(SRCDIR)/parser_mi2.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/parser_mi2.c -o parser_mi2.o $(CFLAGS)

testMI2.o: $(SRCDIR)/testMI2.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/testMI2.c -o testMI2.o $(CFLAGS)

testParser.o: $(SRCDIR)/testParser.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/testParser.c -o testParser.o $(CFLAGS)

realpath.o: $(SRCDIR)/realpath.c
	$(CC) -c $(SRCDIR)/realpath.c -o realpath.o $(CFLAGS)

variables.o: $(SRCDIR)/variables.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/variables.c -o variables.o $(CFLAGS)

debugger.o: $(SRCDIR)/debugger.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/debugger.c -o debugger.o $(CFLAGS)

output.o: $(SRCDIR)/output.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/output.c -o output.o $(CFLAGS)

highlight.o: $(SRCDIR)/highlight.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/highlight.c -o highlight.o $(CFLAGS)

string_parser.o: $(SRCDIR)/string_parser.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/string_parser.c -o string_parser.o $(CFLAGS)

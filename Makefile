# Project: cobgdb
# Makefile originally  created by Dev-C++ 5.11

ifeq ($(OS),Windows_NT)
    WINMODE = 1
    ifeq ($(MSYSTEM),)
        POSIX_UTILS = 0
    else
        POSIX_UTILS = 1
    endif
else
    # detect if running under unix by finding 'rm' in $PATH :
    ifeq ($(wildcard $(addsuffix /rm,$(subst :, ,$(PATH)))),)
        WINMODE = 1
        POSIX_UTILS = 0
    else
        WINMODE = 0
        POSIX_UTILS = 1
    endif
endif

# path where the source resides - same as current Makefile's directory
SRCDIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
OBJ      = cobgdb.o terminal.o read_file.o gdb_process.o parser_mi2.o parser.o mi2.o testMI2.o testParser.o variables.o debugger.o highlight.o string_parser.o util.o

$(info SRCDIR = $(SRCDIR))

ifeq ($(WINMODE),1)
  # Windows
  # objdump -x cobgdb.exe | findstr /R /C:"DLL"
  CC       = gcc.exe
  RES      = 
  BIN      = cobgdb.exe
  OBJ     += realpath.o
  LIBS	 = -static -lwinmm

else

  # Linux
  CC       = gcc
  RES      =
  OBJ     += output.o
  BIN      = cobgdb
  LIBS	 = -lpthread

  # Check whether the file Xlib.h exists in /usr/include/X11/Xlib.h (or in a similar include path, if different on your system).
  X11_HEADER_EXISTS := $(shell [ -f /usr/include/X11/Xlib.h ] && echo yes || echo no)
  ifeq ($(X11_HEADER_EXISTS),yes)
    LIBS     += -lX11
    CPPFLAGS += -DHAVE_X11 -I/usr/include/X11
  endif
endif

# Commands for Windows and Linux
ifeq ($(POSIX_UTILS),0)
  RM = del /F /Q
  CP = copy /Y  
  COBGDB_VERSION := $(shell for /f "tokens=3" %%a in ('findstr /C:"define COBGDB_VERSION" "$(subst /,\,$(SRCDIR)cobgdb.c")') do @echo %%~a)
  COBGDB_VERSION := $(subst ",,$(COBGDB_VERSION))
else
  # --- VERSÃO PARA MSYS / LINUX ---
  RM = rm -f
  CP = cp
  COBGDB_VERSION := $(shell awk '/define COBGDB_VERSION/ {gsub(/"/, "", $$3); print $$3}' $(SRCDIR)cobgdb.c )
endif


CFLAGS   = $(CPPFLAGS) -fdiagnostics-color=always -g -Wall -Wextra

$(info COBGDB_VERSION = $(COBGDB_VERSION))

# Directory and files for distribution
DIST_DIR ?= cobgdb-$(COBGDB_VERSION)
DIST_FILES = $(SRCDIR)README.md \
             $(wildcard $(SRCDIR)*.png) \
             $(wildcard $(SRCDIR)doc/*.pdf) \
             $(SRCDIR)LICENSE

.PHONY: all all-before all-after clean clean-custom copy dist

#=== Default target (executed when running just make) ===
all: all-before $(BIN) all-after

ifeq ($(WINMODE),1)
dist: comp.bat

copy:
	$(CP) $(BIN) windows
	$(CP) comp.bat windows
	$(RM) $(BIN)
endif

dist: $(DIST_DIR) $(BIN) comp.sh $(DIST_FILES)
	$(CP) $(BIN) $(DIST_DIR)
	$(CP) comp.sh $(DIST_DIR)
ifeq ($(WINMODE),1)
	$(CP) comp.bat $(DIST_DIR)
endif
ifeq ($(POSIX_UTILS),0)
	@for %%f in ($(DIST_FILES)) do ( \
		set "src=%%f" && \
		setlocal enabledelayedexpansion && \
		set "src=!src:/=\!" && \
		echo Copiando: !src! && \
		copy "!src!" "$(DIST_DIR)" >nul 2>&1 || echo ERRO ao copiar: !src! && \
		endlocal \
	)
else
	@for f in $(DIST_FILES); do \
		echo "Copiando: $$f"; \
		cp "$$f" "$(DIST_DIR)" || echo "ERRO ao copiar: $$f"; \
	done
endif
	@echo Distribution files copied to $(DIST_DIR)

# Creation of the distribution directory
$(DIST_DIR):
	@mkdir "$(DIST_DIR)"
	
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

util.o: $(SRCDIR)/util.c $(SRCDIR)/cobgdb.h
	$(CC) -c $(SRCDIR)/util.c -o util.o $(CFLAGS)

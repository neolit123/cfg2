# libcfg2 makefile

MAKEFILE = Makefile
CC = gcc
AR = ar
ARFLAGS = rs
CFLAGS = -c -Wall -Wno-write-strings -std=c89 -pedantic -I./include/
LDFLAGS =

ifneq ($(RELEASE), 1)
   CFLAGS += -g
endif

SRCPATH = ./src
OBJPATH = ./obj
SRCFILES = $(wildcard $(SRCPATH)/*.c)
_OBJ = $(patsubst %.c,%.o,$(SRCFILES))
OBJ = $(patsubst $(SRCPATH)/%,$(OBJPATH)/%,$(_OBJ))
OBJ_DYN = $(patsubst %.o,%.dyn.o,$(OBJ))
HEADERS = ./include/cfg2.h ./scr/defines.h
TESTSRC = ./test/test.c
TESTOBJ = ./obj/test.o
LIBNAME = libcfg2
LIBPATH = ./lib
LIBFILE = $(LIBPATH)/$(LIBNAME).a
TESTPATH = ./test

VPATH = $(SRCPATH)

ifeq ($(OS),Windows_NT)
    NULLDEVICE = NUL
    DLLFILE = ./lib/$(LIBNAME).dll
    LIBFILE_DYN = $(DLLFILE).a
    TESTEXE = test.exe
    DLLLINK = -Wl,--out-implib,$(LIBFILE_DYN)
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
else
    NULLDEVICE = /dev/null
    CFLAGS += -fPIC
    DLLFILE = ./lib/$(LIBNAME).so
    LIBFILE_DYN =
    TESTEXE = ./test
    DLLLINK =
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
endif

all: $(LIBFILE) $(DLLFILE) $(TESTPATH_EXE)

$(LIBFILE): $(OBJ)
	@echo building $(LIBFILE)
	@$(AR) $(ARFLAGS) $(LIBFILE) $(OBJ) > $(NULLDEVICE) 2>&1

$(DLLFILE): $(OBJ_DYN)
	@echo building $(DLLFILE)
	@$(CC) -shared $(OBJ_DYN) -o $(DLLFILE) $(DLLLINK)

$(OBJ): $(TESTSRC) $(MAKEFILE)
$(OBJPATH)/%.o: $(SRCPATH)/%.c
	@echo building $@
	@$(CC) -c $< $(CFLAGS) -o $@

$(OBJ_DYN): $(TESTSRC) $(MAKEFILE)
$(OBJPATH)/%.dyn.o: $(SRCPATH)/%.c
	@echo building $@
	@$(CC) -c $< $(CFLAGS) -DCFG_DYNAMIC -o $@

$(TESTPATH_EXE): $(LIBFILE) $(TESTOBJ)
	@echo building $(TESTPATH_EXE)
	@$(CC) $(OBJ) $(TESTOBJ) -o $(TESTPATH_EXE) -L$(LIBPATH) -lcfg2

$(TESTOBJ): $(TESTSRC) $(MAKEFILE)
	@echo building $(TESTOBJ)
	@$(CC) $(CFLAGS) $(TESTSRC) -o $(TESTOBJ)

lib: $(LIBFILE) $(DLLFILE)

test: $(TESTPATH_EXE)

run: $(TESTPATH_EXE)
	cd $(TESTPATH) && $(TESTEXE) && cd ..

clean:
	rm -f $(OBJ) $(OBJ_DYN) $(LIBFILE) $(LIBFILE_DYN) $(DLLFILE) $(TESTPATH_EXE) $(TESTOBJ)

# libcfg2 makefile

MAKEFILE = Makefile
CC = gcc
AR = ar
ARFLAGS = rvs
CFLAGS = -c -Wall -std=c89 -pedantic -I./include/
LDFLAGS =

ifneq ($(RELEASE), 1)
   CFLAGS += -g
endif

SRC = ./src/cfg2.c
HEADERS = ./include/cfg2.h
OBJ = ./obj/cfg2.o
TESTSRC = ./test/test.c
TESTOBJ = ./obj/test.o
OBJ_DYN = ./obj/cfg2.dyn.o
LIBNAME = libcfg2
LIBPATH = ./lib
LIBFILE = $(LIBPATH)/$(LIBNAME).a
TESTPATH = ./test

ifeq ($(OS),Windows_NT)
    DLLFILE = ./lib/$(LIBNAME).dll
    LIBFILE_DYN = $(DLLFILE).a
    TESTEXE = test.exe
    DLLLINK = -Wl,--out-implib,$(LIBFILE_DYN)
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
else
    CFLAGS += -fPIC
    DLLFILE = ./lib/$(LIBNAME).so
    LIBFILE_DYN =
    TESTEXE = ./test
    DLLLINK =
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
endif

all: $(LIBFILE) $(DLLFILE) $(TESTPATH_EXE)

$(LIBFILE): $(OBJ)
	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJ)

$(DLLFILE): $(OBJ_DYN)
	$(CC) -shared $(OBJ_DYN) -o $(DLLFILE) $(DLLLINK)

$(OBJ): $(SRC) $(HEADERS) $(MAKEFILE)
	$(CC) $(CFLAGS) $(SRC) -o $(OBJ)

$(OBJ_DYN): $(SRC) $(HEADERS) $(MAKEFILE)
	$(CC) $(CFLAGS) -DCFG_DYNAMIC $(SRC) -o $(OBJ_DYN)

$(TESTPATH_EXE): $(OBJ) $(TESTOBJ)
	$(CC) $(OBJ) $(TESTOBJ) -o $(TESTPATH_EXE)

$(TESTOBJ): $(TESTSRC) $(MAKEFILE)
	$(CC) $(CFLAGS) $(TESTSRC) -o $(TESTOBJ)

lib: $(LIBFILE) $(DLLFILE)

test: $(TESTPATH_EXE)

run: $(TESTPATH_EXE)
	cd $(TESTPATH) && $(TESTEXE) && cd ..

clean:
	rm -f $(OBJ) $(OBJ_DYN) $(LIBFILE) $(LIBFILE_DYN) $(DLLFILE) $(TESTPATH_EXE) $(TESTOBJ)

install:
	@echo 'make install' not implemented!

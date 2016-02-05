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
    TESTEXE = test.exe
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
else
    CFLAGS += -fPIC
    DLLFILE = ./lib/$(LIBNAME).so
    TESTEXE = ./test
    TESTPATH_EXE = $(TESTPATH)/$(TESTEXE)
endif
LIBFILE_DYN = $(DLLFILE).a

all: $(LIBFILE) $(LIBFILE_DYN) $(DLLFILE) $(TESTPATH_EXE)

$(LIBFILE): $(OBJ)
	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJ)

$(LIBFILE_DYN): $(OBJ_DYN)
	$(AR) $(ARFLAGS) $(LIBFILE_DYN) $(OBJ_DYN)

$(DLLFILE): $(OBJ_DYN)
	$(CC) -shared $(OBJ_DYN) -o $(DLLFILE)

$(OBJ): $(SRC) $(HEADERS) $(MAKEFILE)
	$(CC) $(CFLAGS) $(SRC) -o $(OBJ)

$(OBJ_DYN): $(SRC) $(HEADERS) $(MAKEFILE)
	$(CC) $(CFLAGS) -DCFG_DYNAMIC $(SRC) -o $(OBJ_DYN)

$(TESTPATH_EXE): $(OBJ) $(TESTOBJ)
	$(CC) $(OBJ) $(TESTOBJ) -o $(TESTPATH_EXE)

$(TESTOBJ): $(TESTSRC) $(MAKEFILE)
	$(CC) $(CFLAGS) $(TESTSRC) -o $(TESTOBJ)

lib: $(LIBFILE) $(LIBFILE_DYN)

test: $(TESTPATH_EXE)

run:
	cd $(TESTPATH) && $(TESTEXE) && cd ..

clean:
	rm -f $(OBJ) $(OBJ_DYN) $(LIBFILE) $(LIBFILE_DYN) $(DLLFILE) $(TESTPATH_EXE) $(TESTOBJ)

install:
	@echo 'make install' not implemented!

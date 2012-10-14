CC=gcc
AR=ar
ARFLAGS=rcs
CFLAGS=-c -Wall -I./include
LDFLAGS=-L./lib -lcfg2
LIBFILE=lib/libcfg2.a
TESTEXE=test/test.exe

lib: $(LIBFILE)
all: $(TESTEXE)
test: $(TESTEXE)

$(LIBFILE): src/cfg2.o
	$(AR) $(ARFLAGS) $(LIBFILE) src/cfg2.o

src/cfg2.o: src/cfg2.c include/cfg2.h Makefile
	$(CC) $(CFLAGS) src/cfg2.c -o src/cfg2.o

test/test.o: test/test.c include/cfg2.h Makefile
	$(CC) $(CFLAGS) test/test.c -o test/test.o

$(TESTEXE): $(LIBFILE) test/test.o
	$(CC) test/test.o -o $(TESTEXE) $(LDFLAGS)

clean:
	rm -f src/cfg2.o test/test.o $(LIBFILE) $(TESTEXE)  

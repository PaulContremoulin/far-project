# Makefile

all:
	gcc -L/usr/local/lib validator.c -o valid -ljson

clean:
	rm -f *~ *.o $(BIN)

.SUFFIXES: .c .o

.zf.res:
	$(GCC) -c $<

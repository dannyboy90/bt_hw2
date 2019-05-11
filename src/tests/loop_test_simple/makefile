# loop_test: loop_test.cpp 
	# g++ -o loop_test loop_test.cpp -I.

IDIR =.
CC=g++
CFLAGS=-I$(IDIR) -O0

ODIR=.
LDIR =../lib

LIBS=-lm

_DEPS = 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = loop_testMain.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

loop_test_simple: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
CC=arm-linux-gnueabi-gcc
IDIR=./
CFLAGS=-g -I${IDIR}

ODIR=./
LDIR=./

BINARY=ece531

_DEPS = ${BINARY}.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ${BINARY}.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

${BINARY}: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ ${BINARY}

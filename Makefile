CC=gcc
IDIR=./
CFLAGS=-g -I${IDIR}
LFLAGS=-L/usr/lib/x86_64-linux-gnu
LIBS=-lcurl -lpthread

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
	$(CC) -o $@ $^ $(CFLAGS) ${LFLAGS} $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ ${BINARY}

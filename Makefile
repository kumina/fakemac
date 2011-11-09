CFLAGS=-fPIC -O2 -Wall -Werror -Wstrict-prototypes
LDFLAGS=-ldl

all: libfakemac.so

libfakemac.so: libfakemac.c
	$(CC) -shared -o libfakemac.so libfakemac.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f libfakemac.so

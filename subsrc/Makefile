
.PHONY: default clean

PKG_CFLAGS=$(shell pkg-config --cflags alsa)
PKG_LIBS=$(shell pkg-config --libs alsa)

default: all

clean:
	rm -f *.o microsynth

all: microsynth

microsynth: main.o gen.o synth.o
	gcc -o $@ $^ -pipe $(PKG_LIBS) -lm -lreadline -pthread

%.o: %.c
	gcc -c -o $@ $< $(PKG_CFLAGS) -D_BSD_SOURCE -O2 -Wall -pedantic -pipe -pthread

## dependencies
gen.c: gen.h sampleclock.h
main.c: synth.h
synth.c: synth.h


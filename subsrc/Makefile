
.PHONY: default clean

PKG_CFLAGS=$(shell pkg-config --cflags alsa)
PKG_LIBS=$(shell pkg-config --libs alsa)

default: all

clean:
	rm -f *.o microsynth soundscript_lex.{c,h} soundscript_parse.{c,h}

all: microsynth

microsynth: main.o gen.o synth.o soundscript_lex.o soundscript_parse.o
	gcc -o $@ $^ -pipe $(PKG_LIBS) -lm -lreadline -pthread

%.o: %.c
	gcc -c -o $@ $< $(PKG_CFLAGS) -D_BSD_SOURCE -O2 -Wall -pedantic -pipe -pthread

soundscript_lex.c soundscript_lex.h: soundscript.lex
	flex -o soundscript_lex.c --header-file=soundscript_lex.h soundscript.lex

soundscript_parse.c soundscript_parse.h: soundscript.y
	bison -o soundscript_parse.c --defines=soundscript_parse.h soundscript.y

## dependencies
soundscript_lex.o: soundscript_parse.h
soundscript_parse.o: soundscript_lex.h
gen.o: gen.h sampleclock.h
main.o: synth.h
synth.o: synth.h


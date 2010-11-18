
.PHONY: default clean

default: all

clean:
	rm -f *.o microsynth

all: microsynth

microsynth: main.o gen.o
	gcc -o $@ $^ -pipe -lm -lreadline -pthread

%.o: %.c
	gcc -c -o $@ $< -D_BSD_SOURCE -O2 -Wall -pedantic -pipe -pthread


#CFLAGS = -g -O0

all: io

io: gio.c io_generator.c io_replayer.c
	gcc $(CFLAGS) -o run gio.c io_generator.c io_replayer.c -lrt -pthread

parser: trace_parser.c
	gcc -o parser trace_parser.c
clean:
	rm -rf run
	rm -rf result.txt
	rm -rf test.txt
	rm -rf parser

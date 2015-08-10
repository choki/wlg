#CFLAGS = -g -O0

all: io

io: gio.c io_generator.c io_replayer.c io_aio.c
	gcc $(CFLAGS) -o run gio.c io_generator.c io_replayer.c io_replayer_queue.c common.c io_aio.c -lrt -pthread -laio

parser: trace_parser.c common.c
	gcc -o parser trace_parser.c common.c
clean:
	rm -rf run
	rm -rf result.txt
	rm -rf test.txt
	rm -rf parser

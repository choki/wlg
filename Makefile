#CFLAGS = -g -O0

all:
	gcc $(CFLAGS) -o run workload_generator.c -lrt

parser:
	gcc -o parser trace_parser.c
clean:
	rm -rf run
	rm -rf result.txt
	rm -rf test.txt
	rm -rf parser

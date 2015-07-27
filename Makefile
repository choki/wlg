#CFLAGS = -g -O0

all:
	gcc $(CFLAGS) -o run workload_generator.c -lrt

clean:
	rm -rf run
	rm -rf result.txt
	rm -rf test.txt

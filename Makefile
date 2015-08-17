#CFLAGS = -g -O0
CC = gcc
GIO_TARGET = gio
PARSER_TARGET = parser
LIBS = -lrt -pthread -laio
EXTRA_FILES = *.txt *.log trace trace_p trace.*

GIO_SRCS = gio.c io_generator.c io_replayer.c io_replayer_queue.c common.c io_aio.c io_tracer.c
PARSER_SRCS = trace_parser.c common.c

GIO_OBJS = $(GIO_SRCS:.c=.o)
PARSER_OBJS = $(PARSER_SRCS:.c=.o)

.PHONY : all
all: $(GIO_TARGET) $(PARSER_TARGET)

$(GIO_TARGET): $(GIO_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(PARSER_TARGET): $(PARSER_SRCS)
	$(CC) -o $@ $^
clean:
	rm -rf $(GIO_TARGET) $(PARSER_TARGET)
	rm -rf $(EXTRA_FILES)

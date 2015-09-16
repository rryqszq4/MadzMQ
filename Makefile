CC = gcc -std=gnu99
CFLAGS = -g -O2 -Wall -Wextra

USR_LOCAL = /usr/local

MAD_BROKER = madbroker

COMMON_FILE = src/bstar.c src/kvmsg.c

CLEANUP = rm -f bin/$(MAD_BROKER) src/*.o

all: $(MAD_BROKER)

clean: 
	$(CLEANUP)

LDFLAGS = -L/usr/local/zeromq/lib -L/usr/local/czmq/lib

LIBS = -lzmq -lczmq -luuid

ZMQ_INCLUDES = -I/usr/local/zeromq/include -I/usr/local/czmq/include

$(MAD_BROKER): src/$(MAD_BROKER).c $(COMMON_FILE)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@
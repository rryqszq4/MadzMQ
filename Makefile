CC = gcc -std=gnu99
CFLAGS = -g -O2 -Wall -Wextra

USR_LOCAL = /usr/local

MAD_BROKER = madbroker
MAD_SERVER = madserver

COMMON_FILE = src/aio/bstar.c src/protocols/kvmsg.c
KVMSG = src/protocols/kvmsg.c
BSTAR = src/aio/bstar.c

CLEANUP = rm -f bin/$(MAD_BROKER) bin/$(MAD_SERVER) src/*.o

all: $(MAD_BROKER) $(MAD_SERVER)

clean: 
	$(CLEANUP)

LDFLAGS = -L/usr/local/zeromq/lib -L/usr/local/czmq/lib

LIBS = -lzmq -lczmq -luuid

ZMQ_INCLUDES = -I/usr/local/zeromq/include -I/usr/local/czmq/include

$(MAD_BROKER): src/$(MAD_BROKER).c $(COMMON_FILE)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@

$(MAD_SERVER): src/$(MAD_SERVER).c $(KVMSG)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@
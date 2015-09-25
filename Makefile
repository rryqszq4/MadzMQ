CC = gcc -std=gnu99
CFLAGS = -g -O2 -Wall -Wextra

USR_LOCAL = /usr/local

MAD_BROKER = madbroker
MAD_SERVER = madserver
MAD_CLIENT = madclient
MAD_BROKER_1 = madbroker1

COMMON_FILE = src/aio/bstar.c src/protocols/kvmsg.c
KVMSG = src/protocols/kvmsg.c
BSTAR = src/aio/bstar.c
BROKER = src/broker/mbroker.c src/broker/mbroute.c src/broker/mbpublish.c src/broker/mbqueue.c


CLEANUP = rm -f bin/$(MAD_BROKER) bin/$(MAD_SERVER) bin/$(MAD_CLIENT) bin/$(MAD_BROKER_1) src/*.o

all: $(MAD_BROKER) $(MAD_SERVER) $(MAD_CLIENT) $(MAD_BROKER_1)

clean: 
	$(CLEANUP)

LDFLAGS = -L/usr/local/zeromq/lib -L/usr/local/czmq/lib

LIBS = -lzmq -lczmq -luuid

ZMQ_INCLUDES = -I/usr/local/zeromq/include -I/usr/local/czmq/include

$(MAD_BROKER): src/$(MAD_BROKER).c $(COMMON_FILE)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@

$(MAD_SERVER): src/$(MAD_SERVER).c $(KVMSG)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@

$(MAD_CLIENT): src/$(MAD_CLIENT).c
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@

$(MAD_BROKER_1): src/$(MAD_BROKER_1).c $(COMMON_FILE) $(BROKER)
	$(CC) $(CFLAGS) $(ZMQ_INCLUDES) $(LDFLAGS) $(LIBS) $^ -o bin/$@
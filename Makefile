#
# ICS2305 - Utility Library 
# Code for the ICS2305 Projects
#

# Make environment
INCLUDES=-I.
CC=gcc
CFLAGS=-c -g -Wall $(INCLUDES)
# Files
BINS=server client 
S_OBJS=server.o myqueue.o helper.o
C_OBJS=client.o

# Productions
all : $(BINS)

server: $(S_OBJS)
	$(CC) -pthread -o $@ $^ -lpthread
server.o : tpserver.c helper.h myqueue.h
	$(CC) $(CFLAGS) $< -o $@
helper.o : helper.c helper.h 
	$(CC) $(CFLAGS) $< -o $@
queue.o : myqueue.c myqueue.h 
	$(CC) $(CFLAGS) $< -o $@

client: $(C_OBJS)
	$(CC) -o $@ $^
client.o : client.c
	$(CC) $(CFLAGS) $< -o $@

clean : 
	rm -rf $(BINS) $(S_OBJS) $(C_OBJS)

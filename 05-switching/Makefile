all : switch

CC = gcc
LD = gcc

CFLAGS = -g -Wall -Iinclude
LDFLAGS = 

LIBS = -lpthread

SRCS = main.c mac.c packet.c hash.c

OBJS = $(patsubst %.c,%.o,$(SRCS))

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

switch: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o switch $(LIBS) 

clean:
	rm -f *.o switch

tags: *.c include/*.h
	ctags *.c include/*.h

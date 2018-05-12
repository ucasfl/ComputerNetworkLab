TARGET = tree

all: $(TARGET)

CC = g++
LD = g++

CFLAGS = -g -Wall
LDFLAGS = 

SRCS = main.cpp prefixTree.cpp cvTree.cpp
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

$(OBJS) : %.o : %.cpp ./*.h
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)

obj:
	echo $(OBJS)

clean:
	rm -f *.o $(TARGET)


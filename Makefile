CC = g++
CFLAGS = -std=c++14 -g -O3
SRCS = main cfet shape parse pair util place
OBJS = $(patsubst %, src/%.o, $(SRCS))
LINK = 

.PHONY: all
all: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LINK) -o main -lz3

%.o : %.cpp ./src/cfet.h
	$(CC) $(CFLAGS) $(LINK) -c $< -o $@

.PHONY: clean
clean:
	rm src/*.o
	rm main
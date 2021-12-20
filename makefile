OBJS	= main.o filesystem.o shell.o
SOURCE	= main.c filesystem.c shell.c
HEADER	= filesystem.h shell.h
OUT	= filesystem.out
CC	= gcc
FLAGS	= -g -c -Wall
LFLAGS	= 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) main.c 

filesystem.o: filesystem.c
	$(CC) $(FLAGS) filesystem.c 

shell.o: shell.c
	$(CC) $(FLAGS) shell.c

clean:
	rm -f $(OBJS) $(OUT)
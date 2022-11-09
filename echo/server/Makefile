CC = gcc

TARGET = server

OBJS = echoserveri.o csapp.o

CFLAGS = -Wall -g

LDFLAHS = -lpthread

all: $(TARGET)

$(TARGET):$(OBJS)
    $(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

csapp.o:csapp.c csapp.h
    $(CC) -c csapp.c

echoserveri.o:echoserveri.c echo.c
    $(CC) -c echoserveri.c
	
clean:
    rm -f *.o$(TARGET)


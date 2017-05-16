EXEC = rndpost
OBJS = rndpost.o

CC=gcc -std=gnu99
LIBPATH =
LDFLAGS += $(LIBPATH) -lcurl -lpthread
STRIP = strip
CFLAGS =

all: $(EXEC)
	$(STRIP) $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(EXEC) *.o

PROGS=myshell
OBJS=myshell.o

all: $(PROGS)

%.o: %.c
	gcc -g -pthread -c -o $@ $<

myshell: $(OBJS)
	gcc -g -pthread -o $@ $^

clean:
	rm -rf $(PROGS) *.o

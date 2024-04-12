PROGS=myshell tapper observe reconstruct tapplot
OBJS=myshell.o

all: $(PROGS)

%.o: %.c
	gcc -g -pthread -c -o $@ $<

myshell: $(OBJS)
	gcc -g -pthread -o $@ $^

clean:
	rm -rf $(PROGS) *.o

tapper:
	gcc -g tapper.c -o tapper 

observe:
	gcc -g -o observe observe.c bufferLib.c

reconstruct:
	gcc -g -o reconstruct reconstruct.c bufferLib.c

tapplot:
	gcc -g -o tapplot tapplot.c bufferLib.c
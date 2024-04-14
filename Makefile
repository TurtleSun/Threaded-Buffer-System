PROGS=myshell tapper observe reconstruct tapplot
OBJS=myshell.o

all: $(PROGS)

%.o: %.c
	gcc -g -pthread -c -o $@ $<

myshell: $(OBJS)
	gcc -g -pthread -o $@ $^

clean:
	rm -rf $(PROGS) *.o
	-bash ./clearIPCS.sh

tapper:
	gcc -g -pthread tapper.c -o tapper bufferLib.c

observe:
	gcc -g -pthread -o observe observe.c bufferLib.c

reconstruct:
	gcc -g -pthread -o reconstruct reconstruct.c bufferLib.c

tapplot:
	gcc -g -pthread -o tapplot tapplot.c bufferLib.c
PROGS=myshell tapper observe reconstruct tapplot tappet
OBJS=myshell.o

all: $(PROGS)

%.o: %.c
	gcc -g -pthread -c -o $@ $<

myshell: $(OBJS)
	gcc -g -pthread -o $@ $^

clean:
	rm -rf $(PROGS) *.o
	-bash ./clearIPCS.sh

tapper: tapper.c bufferLibSimplified.c
	gcc -g -pthread tapper.c -o tapper bufferLibSimplified.c

observe: observe.c bufferLibSimplified.c
	gcc -g -pthread -o observe observe.c bufferLibSimplified.c

reconstruct: reconstruct.c bufferLibSimplified.c
	gcc -g -pthread -o reconstruct reconstruct.c bufferLibSimplified.c

tapplot: tapplot.c bufferLibSimplified.c
	gcc -g -o tapplot tapplot.c bufferLibSimplified.c

tappet: tappet.c bufferLib_thread.c observe_thread.c reconstruct_thread.c tapplot_thread.c
	gcc -g -pthread -o tappet tappet.c bufferLib_thread.c observe_thread.c reconstruct_thread.c tapplot_thread.c


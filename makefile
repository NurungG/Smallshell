smallshell: main.o smallshell.o
	gcc -o smallshell main.o smallshell.o

main.o: main.c smallshell.h
	gcc -c main.c

smallshell.o: smallshell.c smallshell.h
	gcc -c smallshell.c

clean:
	rm *.o

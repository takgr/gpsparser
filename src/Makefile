all : gpsparser

gpsparser : gpsparser.c
	gcc -g -c -Wall gpsparser.c -o gpsparser.o -lm
	gcc gpsparser.o -o gpsparser -lm

clean:
	-$(RM) *~ *.o
	-$(RM) gpsparser

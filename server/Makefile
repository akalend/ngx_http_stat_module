all: config.o ini.o server.o parser.o udf.o
	gcc -g -o server parser.o server.o config.o ini.o udf.o

clean:
	rm *.o && rm server

ini.o:
	gcc -g -o ini.o -c ini.c

parser.o:
	gcc -g -o parser.o -c parser.c

config.o:
	gcc -g -o config.o -c config.c

udf.o:
	gcc -g -o udf.o -c udf.c

server.o:
	gcc -g -o server.o -c server.c


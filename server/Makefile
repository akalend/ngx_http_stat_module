all: config.o ini.o server.o parser.o udf.o
	gcc -g -o server parser.o server.o config.o ini.o udf.o ~/src/libdict/bin/libdict.a

clean:
	rm *.o && rm server

ini.o:
	gcc -g -o ini.o -c ini.c

parser.o:
	gcc -g -I/usr/local/include/dict -o parser.o -c parser.c

config.o:
	gcc -g -I/usr/local/include/dict -o config.o -c config.c

udf.o:
	gcc -g -I/usr/local/include/dict -o udf.o -c udf.c

server.o:
	gcc -g  -I/usr/local/include/dict -o server.o -c server.c


# NAME: Ethan Wong
# EMAIL: ethanwong@g.ucla.edu
# ID: 305319001

.SILENT: 
CC = gcc

default:
	$(CC) -g -o lab1b-client -Wall -Wextra lab1b-client.c -lz
	$(CC) -g -o lab1b-server -Wall -Wextra lab1b-server.c -lz

dist:
	tar -czvf lab1b-305319001.tar.gz lab1b-server.c Makefile lab1b-client.c README

clean:
	rm -rf lab1b-client lab1b-server *.tar.gz


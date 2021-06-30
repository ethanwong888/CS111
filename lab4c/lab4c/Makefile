# NAME: Ethan Wong
# EMAIL: ethanwong@g.ucla.edu
# ID: 305319001

.SILENT:

default: 
  gcc -lmraa -lm -Wall -Wextra  -pthread -o lab4c_tcp lab4c_tcp.c
	gcc -lmraa -lm -Wall -lssl -lcrypto -Wextra -pthread -o lab4c_tls lab4c_tls.c

clean:
  rm -rf lab4c_tcp *.tar.gz lab4c_tls

dist:
	tar -zvcf lab4c-305319001.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile
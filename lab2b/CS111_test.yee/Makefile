# NAME: Ethan Wong
# EMAIL: ethanwong@g.ucla.edu
# ID: 305319001

CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -lprofiler

.SILENT:
default:
	$(CC) -g -pthread -lprofiler -Wall -Wextra lab2_list.c SortedList.c -o lab2_list

graphs: tests
	gnuplot lab2b_list.gp 2> /dev/null

clean:
	rm -rf lab2_list *.tar.gz

dist: graphs profile
	tar -czvf lab2b-305319001.tar.gz SortedList.h SortedList.c lab2b_list.csv lab2_list.c lab2b_list.gp lab2b_list.csv profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png  Makefile README

profile: default
	LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=4 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=thread_worker ./lab2_list ./raw.gperf >> profile.out

tests: clean default
	chmod +x tests.sh
	-./tests.sh 2>/dev/null

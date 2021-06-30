// NAME: Ethan Wong
// EMAIL: ethanwong@g.ucla.edu
// ID: 305319001

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "SortedList.h"

int numIterations = 1;
int numThreads = 1;
int numLists = 1;
int yieldFlag = 0;
int opt_yield = 0;
int threadLockTime = 0;
int *spinny;
char option;
char syncType = 'n';
char yieldOpts[8] = "";
struct timespec startTime;
struct timespec endTime;
struct timespec startLockTime;
struct timespec endLockTime;
pthread_mutex_t *mutexLock;
SortedList_t *list;
SortedListElement_t *contents;
long long *threadLocks;

int hash(char key){
  //simple hash of the key, modulo the number of lists
  int h = key % numLists;
  return h;
}

void* threadOperations(void* v){
  //start with a set of pre-allocated and initialized elements(--iterations=#)
  //inserts all the elements into a(single shared-by-all-threads) list
  int begin = *((int*) v);
  int end = numIterations + begin;
  for(int i = begin; i < end; i++){
    int insertIndex = hash(contents[i].key[0]);
    if(syncType == 'n'){
      SortedList_insert(&list[insertIndex], &contents[i]);
    }
    else if(syncType == 'm'){
      int temp = begin / numIterations;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
      pthread_mutex_lock(&mutexLock[insertIndex]);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
      threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) + (endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
      SortedList_insert(&list[insertIndex], &contents[i]);
      pthread_mutex_unlock(&mutexLock[insertIndex]);
    }
    else if(syncType == 's'){
      int temp = begin / numIterations;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
      while(__sync_lock_test_and_set(&spinny[insertIndex], 1)){
	      continue;
      }
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
      threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) + (endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
      SortedList_insert(&list[insertIndex], &contents[i]);
      __sync_lock_release(&spinny[insertIndex]);
    }
  }

  //gets the length of the list
  int length = 0;
	for(int j = 0; j < numLists; j++){
		int lengthIndex = rand() % numLists;
		if(syncType == 'n'){
			length = SortedList_length(&list[lengthIndex]);
		}
		else if(syncType == 'm'){
			int temp = begin / numIterations;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
			pthread_mutex_lock(&mutexLock[lengthIndex]);
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
			threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) + (endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
			length = SortedList_length(&list[lengthIndex]);
			pthread_mutex_unlock(&mutexLock[lengthIndex]);
		}
		else if(syncType == 's'){
			int temp = begin / numIterations;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
			while(__sync_lock_test_and_set(&spinny[lengthIndex], 1)){
				continue;
			}
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
			threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) + (endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
			length = SortedList_length(&list[lengthIndex]);
			__sync_lock_release(&spinny[lengthIndex]);
		}
	}
	if(length < 0){
		fprintf(stderr, "The length of the list revealed it was corrupted.\n");
		exit(2);
	}

  //looks up and deletes each of the keys it had previously inserted
  SortedListElement_t* goodbye;
  for(int i = begin; i < end; i++){
    int deleteIndex = hash(contents[i].key[0]);
    if(syncType == 'n'){
      goodbye = SortedList_lookup(&list[deleteIndex], contents[i].key);
      if(SortedList_delete(goodbye) != 0){
				fprintf(stderr, "Trying to delete from the list revealed it was corrupted.\n");
				exit(2);
			}
    }
    else if(syncType == 'm'){
      int temp = begin / numIterations;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
      pthread_mutex_lock(&mutexLock[deleteIndex]);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
      threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) +(endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
      goodbye = SortedList_lookup(&list[deleteIndex], contents[i].key);
      if(SortedList_delete(goodbye) != 0){
				fprintf(stderr, "Trying to delete from the list revealed it was corrupted.\n");
				exit(2);
			}
      pthread_mutex_unlock(&mutexLock[deleteIndex]);
    }
    else if(syncType == 's'){
      int temp = begin / numIterations;
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startLockTime);
      while(__sync_lock_test_and_set(&spinny[deleteIndex], 1)){
	      continue;
      }
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endLockTime);
      threadLocks[temp] += (endLockTime.tv_nsec - startLockTime.tv_nsec) +(endLockTime.tv_sec - startLockTime.tv_sec) * 1000000000;
      goodbye = SortedList_lookup(&list[deleteIndex], contents[i].key);
      if(SortedList_delete(goodbye) != 0){
				fprintf(stderr, "Trying to delete from the list revealed it was corrupted.\n");
				exit(2);
			}
      __sync_lock_release(&spinny[deleteIndex]);
    }
  }
  return NULL;
}

//in case of a segmentation fault
void sigHandler()
{
	fprintf(stderr, "Found a segmentation fault.\n");
	exit(2);
}

int main(int argc, char **argv){
  //options for getopt_long
	struct option longOptions[] ={
		{"yield", required_argument, 0, 'y'},
		{"iterations", required_argument, 0, 'i'},
		{"sync", required_argument, 0, 's'},
		{"threads", required_argument, 0, 't'},
	 {"lists", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};
	opterr = 0;
  
  //parsing the options -- pretty similar to Lab2A, just add "lists" option
  while(888 == 888){
    option = getopt_long(argc, argv, "", longOptions, NULL);
    //an error occured
		if(option == -1){
			break;
		}
    else if(option == 'y'){
			if(strlen(optarg) > 3){
				fprintf(stderr, "Please only enter up to three arguments for yield.\n");
				exit(1);
			}
			strcat(yieldOpts, "-");
			for(int i = 0; i < (int)strlen(optarg); i++){   //cast to(int) for consistency
				if(optarg[i] == 'i'){
					opt_yield = opt_yield | INSERT_YIELD;
					strcat(yieldOpts, "i");
				}
				else if(optarg[i] == 'd'){
					opt_yield = opt_yield | DELETE_YIELD;
					strcat(yieldOpts, "d");
				}
				else if(optarg[i] == 'l'){
					opt_yield = opt_yield | LOOKUP_YIELD;
					strcat(yieldOpts, "l");
				}
				else{
					fprintf(stderr, "An invalid argument was provided for the yield option.\n");
					exit(1);
				}
			}
		}
    else if(option == 'i'){
			numIterations = atoi(optarg);
		}
    else if(option == 's'){
			syncType = *optarg;
			if(strlen(optarg) != 1){
				fprintf(stderr, "Please provide one argument for sync.\n");
				exit(1);
			}
			if(syncType == 'm'){
				mutexLock = malloc(sizeof(pthread_mutex_t) * numLists);
				for(int j = 0; j < numLists; j++){
					pthread_mutex_init(&mutexLock[j], NULL);
				}
				//pthread_mutex_init(&mutexLock, NULL);
			}
			if(syncType == 's'){
				spinny = malloc(sizeof(int) * numLists);
				for(int j = 0; j < numLists; j++){
					spinny[j] = 0;
				}
			}
		}
    else if(option == 't'){
			numThreads = atoi(optarg);
		}
    else if(option == 'l'){
			numLists = atoi(optarg);
		}
    else{
			fprintf(stderr, "An unrecognized argument was provided.\n");
			exit(1);
		}
  }
  if(!opt_yield){
		strcat(yieldOpts, "-none");
	}

  //output statistics
	char test[99];
	strcat(test, "list");
	strcat(test, yieldOpts);
	if(syncType == 'n'){
		strcat(test, "-none");
	}
	else if(syncType == 'm'){
		strcat(test, "-m");
	}
	else if(syncType == 's'){
		strcat(test, "-s");
	}
	else{
		fprintf(stderr, "An unrecognized sync type was provided.\n");
		exit(1);
	}

  //prepare all the sublists - same as Lab2A, just need a loop for all the sublists now
	list = malloc(sizeof(SortedList_t) * numLists);
	for(int j = 0; j < numLists; j++){
		list[j].key = NULL;
		list[j].prev = &list[j];
		list[j].next = &list[j];
	}

  //same as Lab 2A
	int tempNum = numThreads * numIterations;
	int range = 26;
	int size = 2 * sizeof(char);
	contents = malloc(tempNum * sizeof(SortedListElement_t));
	if(contents == NULL){
		fprintf(stderr, "An error occured with malloc.\n");
		exit(2);
	}
	srand(time(0));
	for(int j = 0; j < tempNum; j++){
		char *k = malloc(size);
		k[0] = 'A' +(rand() % range);
		k[1] = '\0';
		contents[j].key = k;
	}

	signal(SIGSEGV, sigHandler);

  threadLocks = malloc(sizeof(long long) * numThreads);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
  pthread_t *newThreads = malloc(numThreads * sizeof(pthread_t));
  int *spoof = malloc(numThreads * sizeof(int));
	//error occured
	if(newThreads == NULL){
		fprintf(stderr, "An error occured with malloc().\n");
		exit(1);
	}
  //creating threads - same as Lab2A
	for(int j = 0; j < numThreads; j++){
		spoof[j] = j * numIterations;
		if(pthread_create(&newThreads[j], NULL, &threadOperations, &spoof[j])){
			fprintf(stderr, "An error occured when trying to create the threads.\n");
			exit(2);
		}
	}
  //joining threads - same as Lab2A
	for(int j = 0; j < numThreads; j++){
		if(pthread_join(newThreads[j], NULL)){
			fprintf(stderr, "An error occured when trying to join the threads.\n");
			exit(2);
		}
	}
	
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime);

  //calculating total time spent idle waiting for locks
	for(int j = 0; j < numThreads; j++){
		threadLockTime += threadLocks[j];
	}

  long long numOperations = numThreads * 3 * numIterations;
	long long totalTime =(endTime.tv_nsec - startTime.tv_nsec) + (endTime.tv_sec - startTime.tv_sec) * 1000000000;
	int averageTime = totalTime / numOperations;
	int averageThreadLockTime = threadLockTime / numOperations;
  
  //checking if the length is correct
	int l = SortedList_length(list);
	if(l != 0){
		fprintf(stderr, "The final length of the list was not 0.\n");
		exit(2);
	}

  fprintf(stdout, "%s,%d,%d,%d,%lld,%lld,%d,%d\n", test, numThreads, numIterations, numLists, numOperations, totalTime, averageTime, averageThreadLockTime);
	free(list);
	free(contents);
	free(newThreads);
	free(spoof);
	free(spinny);
	free(mutexLock);
  for(int i = 0; i < tempNum; i++){
    free((char*) contents[i].key);
  }
  exit(0);
}
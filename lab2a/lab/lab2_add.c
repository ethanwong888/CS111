// NAME: Ethan Wong
// EMAIL: ethanwong@g.ucla.edu
// ID: 305319001

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>

int numIterations = 1;
int numThreads = 1;
int yieldFlag = 0;
int spinLock = 0;
long long counter = 0;
char syncType = 'n';
char option;
struct timespec startTime;
struct timespec endTime;
pthread_mutex_t mutexLock;

//basic version of add (provided in spec)
void add(long long *pointer, long long value) {
	long long sum = *pointer + value;
	if (yieldFlag){
    sched_yield();
  }
	*pointer = sum;
}

//complicated version of add
void superAdd(int v){
  for(int j = 0; j < numIterations; j++){
    //no synchroinzation
    if(syncType == 'n'){
      add(&counter, v);
    }
    //mutex synchronization
    else if(syncType == 'm'){
      pthread_mutex_lock(&mutexLock);
      add(&counter, v);
      pthread_mutex_unlock(&mutexLock);
    }
    //spin-lock synchronization
    else if(syncType == 's'){
      while (__sync_lock_test_and_set(&spinLock, 1)){
        continue;
      }
      add(&counter, v);
      __sync_lock_release(&spinLock);
    }
    //compare-and-swap synchronization
    else if(syncType == 'c'){
      long long temp1;
      long long temp2;
      temp1 = counter;
			temp2 = temp1 + v;
      if (yieldFlag){
        sched_yield();
      }
      while(__sync_val_compare_and_swap(&counter, temp1, temp2) != temp1){
        temp1 = counter;
        temp2 = temp1 + v;
        if (yieldFlag){
          sched_yield();
        }
      }		
    }
  }
}

void* tempAdd(){
  superAdd(1);
  superAdd(-1);
  return NULL;
}

int main(int argc, char** argv){
  //options for getopt_long
  struct option longOptions[] = {
    {"yield", no_argument, 0, 'y'},
    {"iterations", required_argument, 0, 'i'},
    {"sync", required_argument, 0, 's'},
    {"threads", required_argument, 0, 't'},
    {0, 0, 0, 0}
  };

  //parsing the arguments
  while(888 == 888){
    option = getopt_long(argc, argv, "", longOptions, NULL);
    //an error occured
    if(option == -1){
      break;
    }
    //yield option
    else if (option == 'y'){
      yieldFlag = 1;
      break;
    }
    //iterations option
    else if (option == 'i'){
      numIterations = atoi(optarg);
      if(numIterations < 0){
				fprintf(stderr, "Please give an iteration number that is greater than 0.\n");
				exit(1);
			}
      break;
    }
    //sync option
    else if (option == 's'){
      syncType = *optarg;     //optarg[0]
      if(syncType != 'm' && syncType != 's' && syncType != 'c'){
				fprintf(stderr, "Please enter a valid sync flag.\n");
				exit(1);
			}
      if (syncType == 'm'){
	      pthread_mutex_init(&mutexLock, NULL);
      }
			break;
    }
    //threads option
    else if (option == 't'){
      numThreads = atoi(optarg);
			if(numThreads < 0){
				fprintf(stderr, "Please give a thread number that is greater than 0.\n");
				exit(1);
			}
			break;
    }
    else{
      fprintf(stderr, "An unrecognized option was provided.\n");
			exit(1);
    }
  }

  

  //output statistics
  char* test;
  if (syncType == 'n'){
    if (yieldFlag == 1){
      test = "add-yield-none";
    }
    else{
      test = "add-none";
    }
  }
  else if (syncType == 'm'){
    if (yieldFlag == 1){
      test = "add-yield-m";
    }
    else{
      test = "add-m";
    }
  }
  else if (syncType == 's'){
    if (yieldFlag == 1){
      test = "add-yield-s";
    }
    else{
      test = "add-s";
    }
  }
  else if (syncType == 'c'){
    if (yieldFlag == 1){
      test = "add-yield-c";
    }
    else{
      test = "add-c";
    }
  }
  else{
    fprintf(stderr, "Error: Couldn't find chosen lock type: %c\n", syncType);
  }

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime);
  pthread_t *newThreads = malloc(numThreads * sizeof(pthread_t));
  //error occured
  if(newThreads == NULL){
		fprintf(stderr, "An error occured with malloc().\n");
		exit(1);
	}

  //creating the threads
  for (int j = 0; j < numThreads; j++){
    if (pthread_create(&newThreads[j], NULL, &tempAdd, NULL)) {
      fprintf(stderr, "An error occured when creating the threads.\n");
      exit(1);
    }
  }

  //joining the threads
  for (int j = 0; j < numThreads; j++){
    if (pthread_join(newThreads[j], NULL)){
      fprintf(stderr, "An error occured when joining the threads.\n");
      exit(1);
    }
  }

  //calculating stats for runtimes
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime);
  int numOperations = numThreads * numIterations * 2;
  long long totalTime = (endTime.tv_nsec - startTime.tv_nsec) + ((endTime.tv_sec - startTime.tv_sec) * 1000000000);
  long long averageTime = totalTime / numOperations;
  fprintf(stdout, "%s,%d,%d,%d,%lld,%lld,%lld\n", test, numThreads, numIterations, numOperations, totalTime, averageTime, counter);

  free(newThreads);
  exit(0);
}
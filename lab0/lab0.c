//NAME: Ethan Wong
//EMAIL: ethanwong@g.ucla.edu
//UID: 305319001

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>


//creating a struct with all the various options for getopt_long
struct option longOptions[] = {
  {"input", required_argument, 0, 'i'},
	{"output", required_argument, 0, 'o'},
	{"segfault", no_argument, 0, 's'},
	{"catch", no_argument, 0, 'c'},
	{0, 0, 0, 0}
};

//used to forcibly create a segmentation fault
void createSegmentationFault() {
  char* point = NULL;
  *point = 'x';
}

//handler used with SIGSEGV when a there is a segmentation fault
void handleSegmentationFault() {
  fprintf(stderr,"A segmentation fault has been caught.\n");
	exit(4);
}

int main(int argc, char* argv[]) {
    opterr = 0;
    char* inputFile = NULL;
    char* outputFile = NULL;
    int inputFlag = 0;
    int outputFlag = 0;
    int segFaultFlag = 0;
    int catchFlag = 0;
    char option;

    //processing the option(s)
    while ((option = (getopt_long(argc, argv, "", longOptions, NULL))) != -1){
      //handling all of the different options
      switch(option){
        case 'i':
          inputFile = optarg;
          inputFlag = 1;
          break;
        case 'o':
          outputFile = optarg;
          outputFlag = 1;
          break;
        case 's':
          segFaultFlag = 1;
          break;
        case 'c':
          catchFlag = 1;
          break;
        default:
          fprintf(stderr, "An unrecognized argument was provided. Options are as follows: lab0 --input=filepath --output=filepath --segfault --catch\n");
          exit(1);
      }
    }

    //"catch" flag was activated
    if (catchFlag == 1){
      signal(SIGSEGV, handleSegmentationFault);
    }
    //"segmentationFault" flag was activated
    if (segFaultFlag == 1){
      createSegmentationFault();
    }

    //"input" flag was activated
    if (inputFlag == 1){
      int file = open(inputFile, O_RDONLY);
      //want inputFile to be at 0, because that is where input is traditionally located
      if (file >= 0) {
        close(0);
        dup(file);
        close(file);
      }
      else {
        fprintf(stderr, "File %s could not be opened.\n The error code was %d.\n The error message was %s.\n", inputFile, errno, strerror(errno));
        //more specific error messages for some of the more common errors that a user may encounter
        switch (errno) {
          case EINTR:
            fprintf(stderr,"The file %s could not be opened due to an interrupted function call.\n", inputFile);
            break;
          case EPERM:
            fprintf(stderr,"The file %s could not be opened because the operation is not permitted.\n", inputFile);
            break;
          case EACCES:
            fprintf(stderr,"Permission to file %s was denied.\n", inputFile);
            break;
          case ENOENT:
            fprintf(stderr,"The file %s could not be opened because the specified pathname does not exist.\n", inputFile);
            break;
          case EFAULT:
            fprintf(stderr,"The file %s had a path that led to an inaccessible address.\n", inputFile);
            break;         
          default:
            fprintf(stderr,"The file %s could not be opened because of an unrecognized error.\n", inputFile);
            break;
        }
        exit(2);
      }
    }
    //"output" flag was activated
    if (outputFlag == 1){
      int file = creat(outputFile, 0666);
      //want outputFile to be at 1, because that is where output is traditionally located
      if (file >= 0) {
        close(1);
        dup(file);
        close(file);
      }
      else {
        fprintf(stderr, "File %s could not be opened.\n The error code was %d.\n The error message was %s.\n", outputFile, errno, strerror(errno));
        //more specific error messages for some of the more common errors that a user may encounter
        switch (errno) {
          case EINTR:
            fprintf(stderr,"The file %s could not be opened due to an interrupted function call.\n", inputFile);
            break;
          case EPERM:
            fprintf(stderr,"The file %s could not be opened because the operation is not permitted.\n", inputFile);
            break;
          case EACCES:
            fprintf(stderr,"Permission to file %s was denied.\n", inputFile);
            break;
          case ENOENT:
            fprintf(stderr,"The file %s could not be opened because the specified pathname does not exist.\n", inputFile);
            break;
          case EFAULT:
            fprintf(stderr,"The file %s had a path that led to an inaccessible address.\n", inputFile);
            break;         
          default:
            fprintf(stderr,"The file %s could not be opened because of an unrecognized error.\n", inputFile);
            break;
        }
        exit(3);
      }
    }

    //performing the reading/writing, depending on the flags
    int bytes;
    char operate;
    while(888==888){
      bytes = read(0, &operate, 1);
      if(bytes == 0){
        break;
      }
      if(bytes == -1){
        fprintf(stderr,"An unexpected error has occured when reading: %s; ", strerror(errno));
        exit(5);
      }
      bytes=write(1, &operate, 1);
      if(bytes == -1){
        fprintf(stderr,"An unexpected error has occured when writing: %s; ", strerror(errno));
        exit(6);
      }
    }
    exit(0);
}
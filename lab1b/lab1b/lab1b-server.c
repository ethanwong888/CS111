// NAME: Ethan Wong
// EMAIL: ethanwong@g.ucla.edu
// ID: 305319001

#define _DEFAULT_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <zlib.h>
#include <errno.h>
#include <sys/types.h>

int compressFlag = 0;
int portFlag = 0;
int portNum = 0;
int socketfd;
int secondSocketfd;
unsigned int clilen;   //must be unsigned to prevent warnings
int TS[2];             //pipe for terminal to standard output
int ST[2];             //pipe for standard output to terminal
pid_t pid;
struct sockaddr_in serv_addr;
struct sockaddr_in cli_addr;
int option;
z_stream toClient;
z_stream toServer;


int main(int argc, char** argv){
	//used for getopt_long
	struct option longOptions[] = 
    {
      {"compress", no_argument, 0, 'c'},
      {"port", required_argument, 0, 'p'},             //SHOULD BE REQUIRED_ARGUMENT
      {0, 0, 0, 0}
    };
	opterr = 0;

  //determine which options were selected
	while((option = getopt_long(argc, argv, "", longOptions, NULL)) !=- 1){
    switch(option){
      case 'c':
        compressFlag = 1;
        break;
      case 'p':
        portFlag = 1;
        portNum = atoi(optarg);
        break;
      default:
        fprintf(stderr, "An unrecognized option was provided or an argument is missing.\n");
        exit(1);
        break;
    }
	}
  // --port option is MANDATORY and needed to run properly
	if(portFlag != 1){
		fprintf(stderr, "ERROR: Please include a --port option and argument to use the program.\n");
    exit(1);
	}

  //setting up the sockets for networking
  //NOTE - This code was referenced from the socket programming tutorial provided in the project spec
  //first, create the socket
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0){
    fprintf(stderr, "An error occured when trying to open the socket.\n");
    exit(1);
  }
  
  //next, set up serv_addr struct
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portNum);

  //next, use bind for socketfd and serv_addr
  int bindResult = bind(socketfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindResult < 0){
    fprintf(stderr, "An error occured when trying to bind.\n");
    exit(1);
  }

  //finally, listen for connection request
  listen(socketfd, 5);
  clilen = sizeof(cli_addr);
  secondSocketfd = accept(socketfd, (struct sockaddr *) &cli_addr, &clilen);
  if (secondSocketfd < 0) {
    fprintf(stderr, "An error occured when trying to establish a connection between client and server.\n");
    exit(1);
  }

  
	//checking if pipes were created properly
	if(pipe(ST) == -1 || pipe(TS) == -1){
		fprintf(stderr, "An error occured while trying to create a pipe.\n");
    exit(1);
	}
	
	pid = fork();
  //error occured with fork()
	if(pid == -1) {
		fprintf(stderr, "An error occured while trying to fork a new process.\n");
    exit(1);
	}

  //child process
	else if (pid == 0){
    //have ST take input from TS's 'stdin'
    //have TS output ST's 'stdout' and 'stderr'
    close(TS[1]);
    close(ST[0]);
    dup2(TS[0], 0);
    close(TS[0]);
    dup2(ST[1], 1);
    dup2(ST[1], 2);
    close(ST[1]);

    //create a shell using execvp
    char *cmd = "/bin/bash";
    char *argv[2]; 
    argv[0] = "/bin/bash";
    argv[1] = NULL;
    if (execvp(cmd, argv) == -1) {
      fprintf(stderr, "An error occured when trying to create a shell. Error code: %d. Error message: %s.\n", errno, strerror(errno));
      exit(1);
    }	
	}

  //parent process
	else if(pid > 0){
    //create array of 2 pollfd structures
    //have both pollfds wait for input (POLLIN) or error (POLLHUP, POLLERR) events
		struct pollfd polls[2];
		polls[0].fd = secondSocketfd;
		polls[0].events = POLLIN|POLLHUP|POLLERR;
    close(TS[0]);
		polls[1].fd = ST[0];
		polls[1].events = POLLIN|POLLHUP|POLLERR;
		close(ST[1]);
		
		while(888 == 888){
			int pollResult = poll(polls, 2, 0);
      //error has occured with poll()
			if(pollResult == -1){
        fprintf(stderr, "An error occured when trying to use poll().\n");
        exit(1);
			}

      //FIRST SCENARIO - standard input detected
			else if(POLLIN & polls[0].revents){
        //user DOES NOT WANT compression
				if(compressFlag != 1){
					char buffer[512];
					int readResult = read(secondSocketfd, buffer, 512);
          //error has occured with read()
					if(readResult == -1){
						fprintf(stderr, "An error occured when trying to use read().\n");
            exit(1);
					}

          //process characters one at a time
					for(int j = 0; j < readResult; j++){
            switch(buffer[j]){
              case '\r':
              case '\n':
                write(TS[1], "\n", 1);
                break;
              case 0x04:
                close(TS[1]);
                break;
              case 0x03:
                kill(pid, SIGINT);
                break;
              default: 
                write(TS[1], &buffer[j], 1);
                break;
            }
					}	
				}	

        //user DOES WANT compression
				else if(compressFlag == 1){
					char buffer[512];
					char decompressionBuffer[512];
          
          toServer.zalloc = Z_NULL;
					toServer.zfree = Z_NULL;
					toServer.opaque = Z_NULL;

          //error occured with inflateInit()
					if(inflateInit(&toServer) != Z_OK){
            fprintf(stderr, "An error occured when trying to initiate inflation\n");
            exit(1);
					}

					int readResult = read(secondSocketfd, buffer, 512);
          //error has occured with read()
					if(readResult == -1){
						fprintf(stderr, "An error occured when trying to use read().\n");
            exit(1);
					}
					
					//setting up buffer with zstream
          memset(decompressionBuffer, 512, readResult);
          toServer.avail_in = readResult;
          toServer.avail_out = 512;
          toServer.next_in = (Bytef*) buffer;                 //need to cast to (Bytef*) or else warning
          toServer.next_out = (Bytef*) decompressionBuffer;   //need to cast to (Bytef*) or else warning

          //doing the actual decompression
          inflate(&toServer, Z_SYNC_FLUSH);
          while(toServer.avail_in > 0){
            inflate(&toServer, Z_SYNC_FLUSH);
          } 
          
          int temp = (512 - toClient.avail_out);    //prevents warning with different type comparisons in for loop
          for(int j = 0; j < temp; j++){
            switch(buffer[j]){
              case '\r':
              case '\n':
                write(TS[1], "\n", 1);
                break;
              case 0x04:
                close(TS[1]);
                break;
              case 0x03:
                kill(pid, SIGINT);
                break;
              default:
                write(TS[1], &decompressionBuffer[j], 1);
            }
          }
				}
        inflateEnd(&toServer);
		  }
			

      //SECOND SCENARIO - toClient
			else if (POLLIN & polls[1].revents){
        //user DOES NOT WANT compression
				if (compressFlag != 1){
          char buffer[512];
					int readResult = read(ST[0], buffer, 512);
          //error has occured with read()
					if(readResult == -1){
            fprintf(stderr, "An error occured when trying to use read().\n");
            exit(1);
					}
	
					for(int j = 0; j < readResult; j++){
            //encountered EOF character
						if (buffer[j] == 0x04){
              int oof;
              int waitReturn = waitpid(pid, &oof,0);
              if (waitReturn == -1){
                fprintf(stderr, "An error occured when trying to use waitpid() for shell to terminal.\n");
                exit(1);
              }
              fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(oof), WEXITSTATUS(oof));
              exit(0);
						}
						else{
              write(secondSocketfd, &buffer[j], 1);
            }
					}			
				}
        //user DOES WANT compression
				else if(compressFlag == 1){
          char buffer[512];
          char compressionBuffer[512];

          toClient.zalloc = Z_NULL;
          toClient.zfree = Z_NULL;
          toClient.opaque = Z_NULL;

          //error occured with deflateInit()
          if(deflateInit(&toClient, Z_DEFAULT_COMPRESSION) != Z_OK){
            fprintf(stderr, "An error occured when trying to initiate deflation\n");
            exit(1);
          }
					
          int readResult=read(ST[0], buffer, 512);
          //error has occured with read()
          if(readResult == -1){
            fprintf(stderr, "An error occured when trying to use read().\n");
            exit(1);
          }
          
          //setting up buffer with zstream
          memset(compressionBuffer, 0, readResult);
          toClient.avail_in = readResult;
          toClient.avail_out = 512;
          toClient.next_in = (Bytef*) buffer;               //need to cast to (Bytef*) or else warning
          toClient.next_out = (Bytef*) compressionBuffer;   //need to cast to (Bytef*) or else warning
          
          //doing the actual compression
          deflate(&toClient, Z_SYNC_FLUSH);
          while(toClient.avail_in > 0){
            deflate(&toClient, Z_SYNC_FLUSH);
          }
          deflateEnd(&toClient);
          int GORILLA = 512 - toClient.avail_out;
          write(secondSocketfd, &compressionBuffer, GORILLA);

          for(int j = 0; j < readResult; j++){
            //encountered EOF character
            if(buffer[j] == 0x04){
              int oof;
              int waitReturn = waitpid(pid, &oof,0);
              if (waitReturn == -1){
                fprintf(stderr, "An error occured when trying to use waitpid() for shell to terminal.\n");
                exit(1);
              }
              fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(oof), WEXITSTATUS(oof));
              exit(0);
            }
          }
				}
			}

      //THIRD SCENARIO - looking for POLLERR or POLLHUP
      else if ((POLLERR & polls[0].revents) || (POLLHUP & polls[0].revents) || (POLLHUP & polls[1].revents) || (POLLERR & polls[1].revents)){
        int oof;
        int waitReturn = waitpid(pid, &oof, 0);
        if(waitReturn == -1){
          fprintf(stderr, "An error occured when trying to use waitpid().\n");
          exit(1);
        }
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(oof), WEXITSTATUS(oof));
        exit(0);
      }
	  }
  }
  exit(0);
}
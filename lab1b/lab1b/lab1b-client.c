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

struct termios primary;
struct termios secondary;
int compressFlag = 0;
int portFlag = 0;
int portNum = 0;
int logFlag=0;
int logfd;
int option;
char* logFile;
int socketfd;
struct sockaddr_in serv_addr;
struct hostent *server;
z_stream toServer;
z_stream toClient;


int main(int argc, char** argv){
	//used for getopt_long
	struct option longOptions[] = 
    {
      {"compress", no_argument, 0, 'c'},
      {"port", required_argument, 0, 'p'},            //SHOULD BE REQUIRED_ARGUMENT
      {"log", required_argument, 0, 'l'},
      {0, 0, 0, 0}
    };
	opterr = 0;

	//determine which options were selected
  while((option = getopt_long(argc, argv, "", longOptions, NULL)) != -1){
    if(option == 'c'){
      compressFlag = 1;
    }
    else if(option == 'p'){
      portFlag = 1;
      portNum = atoi(optarg);
    }
    else if(option == 'l'){
      logFlag = 1;
      logFile = optarg;
      logfd = open(optarg, O_CREAT | O_WRONLY, 0666);
      if(logfd < 0){
        fprintf(stderr, "An error occured when trying to create a log file.\n");
        exit(1);
      }
    }
    else{
      fprintf(stderr, "An unrecognized option was provided or an argument is missing.\n");
      exit(1);
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
  if (socketfd < 0) {
    fprintf(stderr, "An error occured when trying to open the socket.\n");
    exit(1);
  }

  //next, retrieve the server name
	server = gethostbyname("localhost");    //LOCALHOST???
	if(server == NULL){		
		fprintf(stderr, "An error occured when trying retrieve the server name.\n");
    exit(1);
	}
	
  //next, set up serv_addr struct
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portNum);

  //finally, sttempt to set up the connection between client and server
  int connectResult = connect(socketfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (connectResult < 0) {
    fprintf(stderr, "An error occured when trying to establish a connection to the server.\n");	
    exit(1);
	}
	
  //setting up the terminal properly (from Lab 1A)
	tcgetattr(0, &primary);
	tcgetattr(0, &secondary);
	secondary.c_iflag = ISTRIP;	
	secondary.c_oflag = 0;
	secondary.c_lflag = 0;
	
  //check for error from setting attributes
	if(tcsetattr(0, TCSANOW, &secondary) == -1){
		fprintf(stderr, "An error occured when trying to set the attributes.\n");
		exit(1);
	}
	//handling when user DOES choose compression option (HARDER option) :((((
	if (compressFlag == 1){
    //setting up the polls properly
		struct pollfd polls[2];
		polls[0].fd = 0;
		polls[0].events = POLLIN|POLLHUP|POLLERR;
		polls[1].fd = socketfd;
		polls[1].events = POLLIN|POLLHUP|POLLERR;

		while(888 == 888){
			int pollResult = poll(polls, 2, 0);
			//error has occured with poll()
      if(pollResult == -1){
        fprintf(stderr, "An error occured when trying to use poll().\n");
        exit(1);
      }

      //FIRST SCENARIO - standard input detected
			else if(POLLIN & polls[0].revents){
				char buffer[512];
				char compressionBuffer[512];

				memset(compressionBuffer, 0, 512);
				toServer.zalloc = Z_NULL;
        toServer.opaque = Z_NULL;
				toServer.zfree = Z_NULL;

        //error occured with deflateInit()
				if(deflateInit(&toServer,Z_DEFAULT_COMPRESSION) != Z_OK){
					fprintf(stderr, "An error occured when trying to start deflation\n");
          exit(1);
				}

				int readResult = read(0, buffer, 512);
        //error has occured with read()
				if(readResult == -1){
					fprintf(stderr, "An error occured when trying to use read().\n");
          exit(1);
				}

				//setting up buffer with zstream
        toServer.avail_in = readResult;
        toServer.avail_out = 512;
        toServer.next_in = (Bytef*) buffer;               //need to cast to (Bytef*) or else warning
        toServer.next_out = (Bytef*) compressionBuffer;   //need to cast to (Bytef*) or else warning
				

        //doing the actual compression
        deflate(&toServer, Z_SYNC_FLUSH);
        while(toServer.avail_in > 0){
          deflate(&toServer, Z_SYNC_FLUSH);
        }
				deflateEnd(&toServer);

        int temp = 512 - toServer.avail_out;
				if(logFlag == 1){
          int insert = 5;
          char prefix[14] = "SENT   bytes: ";
          prefix[insert] = '0' + temp;
          write(logfd, prefix, 14);
          write(logfd, compressionBuffer, temp);
          write(logfd, "\n", 1);
				}
				write(socketfd, &compressionBuffer, temp);
				

				for(int j = 0; j < readResult; j++){
          switch(buffer[j]){
            case '\r':
            case '\n':
              write(1, "\r\n", 2);
              break;
            default:
              write(1, &buffer[j],1);
              break;
          }
				}
			}				
			
      //SECOND SCENARIO - input from socket detected
			else if(POLLIN & polls[1].revents){
				unsigned char buffer[512];
				unsigned char decompressionBuffer[512];

        memset(decompressionBuffer, 0, 512);
				toClient.zalloc = Z_NULL;
        toClient.zfree = Z_NULL;
        toClient.opaque = Z_NULL;

        //error occured with inflateInit()
        if (inflateInit(&toClient) != Z_OK) {
          fprintf(stderr, "An error occured when trying to start inflation\n");
          exit(1);
        }

				int readResult = read(socketfd,buffer,512);
        //error has occured with read()
				if(readResult == -1){
					fprintf(stderr, "An error occured when trying to use read().\n");
          exit(1);
				}
        //EOF, shut down accordingly
        else if(readResult == 0){
          if(logFlag){
            close(logfd);
          }
					tcsetattr(0, TCSANOW, &primary);
          close(socketfd);
					exit(0);
				}
				
        //setting up buffer with zstream
        toClient.avail_in = readResult;
        toClient.avail_out = 512;
        toClient.next_in = (Bytef*) buffer;               //need to cast to (Bytef*) or else warning
        toClient.next_out = (Bytef*) decompressionBuffer;   //need to cast to (Bytef*) or else warning

				if(logFlag == 1){
          int insert = 9;
          char beginpart[18] = "RECEIVED   bytes: ";
          beginpart[insert] = '0' + readResult;
          write(logfd, beginpart, 18);
          write(logfd, buffer, readResult);
          write(logfd, "\n", 1);
				}
				
        //doing the actual decompression
        inflate(&toClient, Z_SYNC_FLUSH);
        while(toClient.avail_in > 0){
          inflate(&toClient, Z_SYNC_FLUSH);
        }
				
        //handling any leftover data
				int temp = 512 - toClient.avail_out;   //must do this to prevent warning with comparisons
				for(int j = 0; j < temp; j++){
          switch(buffer[j]){
            case '\r':
            case '\n':
              write(1, "\r\n", 2);
              break;
            default:
              write(1, &decompressionBuffer[j], 1);
              break;
          }
				}
        inflateEnd(&toClient);
			}
			
			//THIRD SCENARIO: checking for POLLERR/POLLHUP
			else if ((POLLERR & polls[1].revents) || (POLLHUP & polls[1].revents)){
				//pretty much just exit normally here
				tcsetattr(0, TCSANOW, &primary);	
				exit(0);	
			}
		}
	}

	//handling when user DOES NOT choose compression option (EASIER option)
	else if(compressFlag != 1){
    //setting up the polls properly
    struct pollfd polls[2];
    polls[0].fd = 0;
    polls[0].events = POLLIN|POLLHUP|POLLERR;
    polls[1].fd = socketfd;
    polls[1].events = POLLIN|POLLHUP|POLLERR;

    while(888 == 888){
      int pollResult = poll(polls, 2, 0);
      //error has occured with poll()
      if(pollResult == -1){
        fprintf(stderr, "An error occured when trying to use poll().\n");
        exit(1);
      }

      //FIRST SCENARIO - standard input detected
      if (POLLIN & polls[0].revents){
        char buffer[512];
			  int readResult = read(0, buffer, 512);
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
              write(socketfd, &buffer[j], 1);
              write(1, "\r\n", 2);
              break;
            default:
              write(socketfd, &buffer[j], 1);
              write(1, &buffer[j], 1);
              break;
          }
        }

        //filling up the log file
        if(logFlag == 1){
          int insert = 5;
          char prefix[14] = "SENT   bytes: ";
          prefix[insert] = '0' + readResult;
          write(logfd, prefix, 14);
          write(logfd, buffer, readResult);
          write(logfd, "\n", 1);
        }
      }
      
      
      //SECOND SCENARIO - input from socket detected
      else if (POLLIN & polls[1].revents){
        char buffer[512];    
        int readResult = read(socketfd, buffer, 512);
        //error has occured with read()
        if(readResult == -1){
          fprintf(stderr, "An error occured when trying to use read().\n");
          exit(1);
        }
        //End of File, handle shutdown accordingly (sanity check freezes without this?)
        else if(readResult == 0){
          if(logFlag){
            close(logfd);
          }
          tcsetattr(0, TCSANOW, &primary);
          close(socketfd);
          exit(0);
        }

        //process characters one at a time
        for(int j = 0; j < readResult; j++){
          switch(buffer[j]){
            case '\r':
            case '\n':
              write(1, "\r\n", 2);
              break;
            default:
              write(1, &buffer[j], 1);
              break;
          }
        }
        
        //filling up the log file
        if(logFlag == 1){
          int insert = 9;
          char prefix[18] = "RECEIVED   bytes: ";
          prefix[insert] = '0' + readResult;
          write(logfd, prefix, 18);
          write(logfd, buffer, readResult);
          write(logfd, "\n", 1);
        }
      }	

      //THIRD SCENARIO: checking for POLLERR/POLLHUP
      else if ((POLLERR & polls[1].revents) || (POLLHUP & polls[1].revents)){
        //pretty much just exit normally here
        tcsetattr(0, TCSANOW, &primary);	
        exit(0);   
      }
    }	
	}				
	tcsetattr(0, TCSANOW, &primary);			
	exit(0);
}
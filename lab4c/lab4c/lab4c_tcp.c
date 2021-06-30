#include <getopt.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <math.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <assert.h>
#include <mraa/gpio.h>


//general
sig_atomic_t volatile runningFlag = 1;
int option;
int readInfo;
int dp;
int id;
int startFlag = 1;
int periodTime = 1;
int logFlag = 0;
int logFile = 0;
int i;
int j;
int spoof;
int portNum;
int wocket;
int serverSize;
size_t bufferLength;
struct pollfd polls;
time_t startTime;
pthread_t *threads;
struct tm *timingInfo;
struct hostent *serverName;
struct sockaddr_in server;
mraa_gpio_context button;
mraa_aio_context tempSensor;
float temperature;
int scaleFlag = 'F';
char *hostName;
char commandBuffer[512];
char userInputBuffer[512];
char printBuffer[512];


void tempConversion(int raw, float *t){
  float val = (float)(1023 - raw) * 10000 / raw;
  *t = 1 / (log(val / 10000) / 3975 + 1 / 298.15) - 273.15;
  if(scaleFlag == 'F'){
    *t = *t * 1.8 + 32;
  }
}

void *commands(){
	while(runningFlag){
		poll(&polls, 1, 0);
		if(POLLIN & polls.revents){
			dp = 0;
			//read the user's input
			readInfo = read(wocket, userInputBuffer, 512);
			for(i = 0; userInputBuffer[i] != '\0'; i++){
				if(userInputBuffer[i] == '\n'){
					//place into commandBuffer
					memset(commandBuffer, 0, 512);
					size_t n = (1 + i) - dp;
					memcpy(commandBuffer, &userInputBuffer[dp], n);
					
					//START
          if(strstr(commandBuffer,"START")){
            startFlag = 1;
          }
					//STOP
          else if(strstr(commandBuffer,"STOP")){
            startFlag = 0;
          }
					//OFF
          else if(strstr(commandBuffer,"OFF")){
            runningFlag = 0;
          }
					//Period (seconds)
          else if(commandBuffer[0] == 'P' && commandBuffer[1] == 'E' &&commandBuffer[2] == 'R' && commandBuffer[3] == 'I' && commandBuffer[4] == 'O' && commandBuffer[5] == 'D' && commandBuffer[6] == '='){  
            periodTime = (commandBuffer[7]);
          }
					//Fahrenheit Scale
          else if(commandBuffer[0] == 'S' && commandBuffer[1] == 'C' &&commandBuffer[2] == 'A' && commandBuffer[3] == 'L' && commandBuffer[4] == 'E' && commandBuffer[5] == '=' && commandBuffer[6] == 'F'){
            scaleFlag = 'F';
          }
          //Celsius Scale
          else if(commandBuffer[0] == 'S' && commandBuffer[1] == 'C' &&commandBuffer[2] == 'A' && commandBuffer[3] == 'L' && commandBuffer[4] == 'E' && commandBuffer[5] == '=' && commandBuffer[6] == 'C'){
            scaleFlag = 'C';
          }
					//LOG
          else if(strstr(commandBuffer,"LOG")){
            ;
          }
					if(logFlag){
						write(logFile, commandBuffer, strlen(commandBuffer));
					}
					dp = i+1;	
				}
			}
			memset(userInputBuffer, 0, 512);
		}
		//check for polling errors
    else if((POLLIN | POLLERR | POLLHUP) & polls.revents){
      fprintf(stderr, "An error occured while polling.\n");
      exit(1);
    }
	}
	return 0;
}

void *sensorPoll(){
	//taking note of the time during setup
	int elapsedTime = 0;
	time(&startTime);
	timingInfo = localtime(&startTime);
	int seconds = timingInfo->tm_sec;
	
	//while the sensor is operating
	while(runningFlag){
		time(&startTime);
		timingInfo = localtime(&startTime);
		//increment elapsedTime (elapsed seconds) and update 'seconds' if there has been a change
		if(seconds != timingInfo->tm_sec){
			elapsedTime += 1;
			seconds = timingInfo->tm_sec;
		}
		//if elapsedTime >= period (number of seconds between reporting intervals)
		if(elapsedTime >= periodTime){
			tempConversion(mraa_aio_read(tempSensor), &temperature);
			int sec = timingInfo->tm_sec;
			int min = timingInfo->tm_min;
			int hour = timingInfo->tm_hour;
			memset(printBuffer, 0, 512);
			sprintf(printBuffer, "%02d:%02d:%02d %.1f\n", hour, min, sec, temperature);

			//writing to logFile and printing (if necessary)
			if((startFlag == 1) && (runningFlag == 1)){
				if(logFlag == 1){
					bufferLength = strlen(printBuffer);
					write(logFile, printBuffer, bufferLength);
				}
				dprintf(wocket, printBuffer);
			}
			elapsedTime = 0;
		}
	}
  return 0;
}

void shutdownProcess(){
  runningFlag = 0;
}

int main(int argc, char *argv[]){
	struct option longOptions[] = {
		{"period", required_argument, NULL, 'p'},
    {"scale", required_argument, NULL, 's'},
    {"log", required_argument, NULL, 'l'},
		{"id", required_argument, NULL, 'i'},
		{"host", required_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};
	opterr = 0;
	while((option = getopt_long(argc, argv, "", longOptions, 0)) != -1){
		//parsing based on what options user wants
		switch(option){
			//log option
			case 'l':
				logFlag = 1;
				logFile = creat(optarg, 0666);
				break;
			//scale option
			case 's':
				if(*optarg == 'F'){
					scaleFlag = 'F';
				}  
				else if(*optarg == 'C'){
					scaleFlag = 'C';
				}
				else{
					fprintf(stderr, "Please choose either F (Fahrenheit) or C (Celsius).\n");
					exit(1);
				}
				break;
			//period option
			case 'p':
				periodTime = atoi(optarg);
				break;
			//host option
			case 'h':
				hostName = optarg;
				break;
			//ID option
			case 'i':
				id = atoi(optarg);
				break;
			default:
				fprintf(stderr, "An invalid option was provided or an argument was missing.\n");
				exit(1);
		}
	}

	if(argc > optind){
		for(j = 1; j < argc; j++){
			(argv[j][0] != '-') ? (portNum = atoi(argv[j])) : (portNum = portNum);
		}
	}

	//setting up the pins
	button = mraa_gpio_init(60);
	tempSensor = mraa_aio_init(1);
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &shutdownProcess, NULL);

	//setting up the socket
	wocket = socket(AF_INET, SOCK_STREAM, 0);
	serverName = gethostbyname(hostName);
	if((serverName == NULL) || (wocket == -1)){
		fprintf(stderr, "An error occured when trying to set up the socket.\n");
		exit(0);
	}

	//setting up and attempting connection (pretty much same as Lab 1B)
	serverSize = sizeof(server);
	memset((char*) &server, 0, serverSize);
	server.sin_family = AF_INET;
	memcpy((char*) &server.sin_addr.s_addr, (char*)serverName->h_addr, serverName->h_length);
	server.sin_port = htons(portNum);
	if(connect(wocket, (struct sockaddr*)&server, serverSize) < 0){
		fprintf(stderr, "An error occured when trying to connect.\n");
		exit(1);
	}

	//ID
	memset(printBuffer, 0, 512);
	sprintf(printBuffer, "ID=%d\n", id);
	dprintf(wocket, printBuffer);
	if(logFlag == 1){
		bufferLength = strlen(printBuffer);
		write(logFile, printBuffer, bufferLength);
	}

	//setting up polls
	polls.fd = wocket;
	polls.events = POLLIN|POLLERR|POLLHUP;
	polls.revents = 0;

	//multithreading
	threads = malloc(sizeof(pthread_t)*2);
	pthread_create(&threads[0], NULL, sensorPoll, NULL);
	pthread_create(&threads[1], NULL, commands, NULL);
	pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

	//shutting down
	time(&startTime);
	timingInfo = localtime(&startTime);
	memset(printBuffer, 0, 512);
	int sec = timingInfo->tm_sec;
  int min = timingInfo->tm_min;
  int hour = timingInfo->tm_hour;
  sprintf(printBuffer, "%02d:%02d:%02d SHUTDOWN\n", hour, min, sec);
	if(logFlag == 1){
		bufferLength = strlen(printBuffer);
		write(logFile, printBuffer, bufferLength);
	}
	dprintf(wocket, printBuffer);
	close(logFile);
	mraa_aio_close(tempSensor);
	mraa_gpio_close(button);
	exit(0);
}
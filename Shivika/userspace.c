/*
* Original Code taken from Dereck Molloy (text: Exploring Beagle Board Black)
* Modified for this assignment by Shivika Malik
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#define NUM_THREADS 2

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

void *WorkerThread(void *threadid)
{
  int str, file, threadst;
  char string[BUFFER_LENGTH];
  file = open("/dev/leddriver",O_RDWR);    //To open the device with read & write access
  if (file < 0)
	perror("Failed to open the device !!");
  Switch: printf("Type the string to turn on the 7 Segment:\n"); //Turning On or Off the LEDs and asking the user for new input until '-1' is entered
  printf("Type -1 to exit.\n");
  Scan: scanf("%[^\n]%*c", string);	//Scans the string that is to be sent to the device
	if(string[0] == '-' && string[1]=='1'){	//Checks for '-1', if -1 then the code exits
	  int err = close(file);
		if (err < 0){
			perror("Error closing file.");}
		pthread_exit(NULL);
	}
  
	else{			//Writes the string to the device
	  str = write(file, string, strlen(string));
	  if(str < 0)
		  perror("Failed to write the message to the device.");
	  goto Switch;
  }	
}


int main(int arg_1, char *arg_2[]){

pthread_t threads[1];
int threadst;
printf("Starting device test.\n"); 	//Device Start
threadst = pthread_create(&threads[0], NULL, WorkerThread, (void *)0);	//Creates thread
if(threadst)
{
	printf("ERROR !! Returning the error.");
	exit(1);
}
pthread_join(threads[0], NULL);
return 0;
}


#include <iostream>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr) {
	// Generate a unique key for shared memory + message queue. 
	key_t key = ftok("keyfile.txt", 'a');
	
	// Check for successfull key generation.
	if (key == -1) {
		perror("Error: unable to generate key.");
		exit(-1);
	}

	/* Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666|IPC_CREAT);

	// Check for succesfull memory allocation.
	if (shmid == -1) {
		perror("Error: unable to find memory segment.");
		exit(-1);
	}
	
	/* Attach to the shared memory */
	sharedMemPtr = shmat(shmid, (void*)0, 0);

	// Check for successful attachment onto shared memory.
	if (sharedMemPtr == (void*)-1) {
		perror("Error: unable to attach onto shared memory.");
		exit(-1);
	}

	/* Attach to the message queue */
	msqid = msgget(key, 0666|IPC_CREAT);
	
	// Check for successfully getting the message queue id.
	if (msqid == -1) {
		perror("Error: unable to find the message queue.");
		exit(-1);
	}
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr) {
	/* Detach from shared memory */
	if (shmdt(sharedMemPtr) == -1) {
		perror("Error: unable to detach from shared memory.");
		exit(-1);
	}
}

/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");
	

	/* A buffer to store message we will send to the receiver. */
	message sndMsg; 
	
	/* A buffer to store message received from the receiver. */
	message rcvMsg;
	
	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}
	
	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}
		
			
		/* Send a message to the receiver telling him that the data is ready 
 		 * (message of type SENDER_DATA_TYPE) 
 		 */

		 sndMsg.mtype = SENDER_DATA_TYPE;

		 if (msgsnd(msqid, &sndMsg, sizeof(sndMsg), 0) == -1){
			 perror("Error: sender unable to send data ready message.");
			 exit(-1);
		 }
		 std::cout << "Message sent: " << sndMsg.size << " bytes." << std::endl;
		
		/* Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us 
 		 * that he finished saving the memory chunk. 
 		 */

		 if (msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), RECV_DONE_TYPE, 0) == -1){
			 perror("Error: sender unable to receive message.");
			 exit(-1);
		 }
	}
	

		/* 
		*	Once we are out of the above loop, we have finished sending the file.
 	  * Lets tell the receiver that we have nothing more to send. We will do this by
 	  * sending a message of type SENDER_DATA_TYPE with size field set to 0. 	
	  */
		
	sndMsg.size = 0;
	if (msgsnd(msqid, &sndMsg, sizeof(sndMsg), 0) == -1){
		perror("Error: sender unable to send finish message.");
	 	exit(-1);
	}
	std::cout << "Message sent: " << sndMsg.size << " bytes." << std::endl;

	/* Close the file */
	fclose(fp);
	
}


int main(int argc, char** argv)
{
	
	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}
		
	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);
	
	/* Send the file */
	send(argv[1]);
	
	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);
		
	std::cout << "Process finished execution." << std::endl;
		
	return 0;
}

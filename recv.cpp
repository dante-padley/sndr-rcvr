// Eli Manzo-de Leon : 3/13/20
// This file is responsible for receiving messages from the sender file
// until the message received has a size fo 0. Once a message has been received,
// the receiver will write the amount of data sent to the file recvfile.

#include <iostream>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */


/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";


/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr) {
	 
	// Generate a unique key for shared memory + message queue. 
	key_t key = ftok("keyfile.txt", 'a');
	
	// Check for successfull key generation.
	if (key == -1) {
		perror("Error: unable to generate key.");
		exit(-1);
	}

	
	/* Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666|IPC_CREAT);
	
	// Check for succesfull memory allocation.
	if (shmid == -1) {
		perror("Error: unable to allocate shared memory.");
		exit(-1);
	}
	
	
	/* Attach to the shared memory */
	sharedMemPtr = shmat(shmid, (void*)0, 0);
	
	// Check for successful attachment onto shared memory.
	if (sharedMemPtr == (void*)-1) {
		perror("Error: unable to attach onto shared memory.");
		exit(-1);
	}

	
	/* Create a message queue */
	msqid = msgget(key, 0666|IPC_CREAT);
	
	// Check for successful message queue creation.
	if (msqid == -1) {
		perror("Error: unable to create a message queue.");
		exit(-1);
	}	
}
 

/**
 * The main loop
 */
void mainLoop()
{
	/* The size of the mesage */
	int msgSize = 0;
	
	/* Receive message */
	message rcvMsg;
	
	/* Send message */
	message sndMsg;
	
	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");
		
	/* Error checks */
	if (!fp) {
		perror("fopen");	
		exit(-1);
	}
		
    /* Receive the message and get the message size. The message will 
     * contain regular information. The message will be of SENDER_DATA_TYPE
     * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
     * of the message is not 0, then we copy that many bytes from the shared
     * memory region to the file. Otherwise, if 0, then we close the file and
     * exit.
     *
     * NOTE: the received file will always be saved into the file called
     * "recvfile"
     */
		
	if (msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), SENDER_DATA_TYPE, 0) == -1) {
		perror("Error: receiver unable to receive first message.");
		exit(-1);
	}

	// Update the message size.
	msgSize = rcvMsg.size;
	
	
	/* Keep receiving until the sender set the size to 0, indicating that
 	 * there is no more data to send
 	 */
	
	

	while(msgSize != 0) {	
		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0) {
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0) {
				perror("fwrite");
			}

			// Set message type to RECV_DONE_TYPE.
			sndMsg.mtype = RECV_DONE_TYPE;
			
			// Send message and check if it was successfully sent.
			if (msgsnd(msqid, &sndMsg, sizeof(sndMsg), 0) == -1) {
				perror("Error: receiver unable to send confirmation message.");
				exit(-1);
			}

			// Check for message from sender.
			if (msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), SENDER_DATA_TYPE, 0) == -1) {
				perror("Error: receiver unable to receive message.");
				exit(-1);
			}
	
			
			// Update the message size.
			msgSize = rcvMsg.size;
						
		} else {
			/* Close the file */
			fclose(fp);
		}
	}
}

/*
 * Perfoms the cleanup functions
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
	
	/* Deallocate the shared memory chunk */
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("Error: unable to deallocate shared memory.");
		exit(-1);
	}
	
	/* Deallocate the message queue */
	if (msgctl(msqid, IPC_RMID, NULL) == -1) {
		perror("Error: unable to deallocate message queue.");
		exit(-1);
	}
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal) {
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv) {
	/* Signal Handler */
	signal(SIGINT, ctrlCSignal);
				
	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	
	/* Go to the main loop */
	mainLoop();

	/** Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) **/
	cleanUp(shmid, msqid, sharedMemPtr);
	
	std::cout << "Receiver finished execution." << std::endl;
	
	return 0;
}

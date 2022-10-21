/**************************************************************
* Class:  CSC-415-0# Fall 2022
* Name: Jiayi Gu
* Student ID: 920024739
* GitHub UserID: Jiayi079
* Project: Assignment 5 – Buffered I/O
*
* File: b_io.c
*
* Description:
*
*		This assignment is to handle the input and output when
*		we are doing the buferring. We will use the provide
*		skeleton code frame to writeing three function:
*		b_io_fd b_open (char * filename, int flags);
*		int b_read (b_io_fd fd, char * buffer, int count);
*		int b_close (b_io_fd fd);
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20	//The maximum number of files open at one time


// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb
	{
	fileInfo * fi;	//holds the low level systems file info

	// Add any other needed variables here to track the individual open file
	char * buffer;
	int byteCopied; // the number of bytes already input to the buffer

	} b_fcb;
	
//static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;	

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init ()
	{
	if (startup)
		return;			//already initialized

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fi = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free File Control Block FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fi == NULL)
			{
			fcbArray[i].fi = (fileInfo *)-2; // used but not assigned
			return i;		//Not thread safe but okay for this project
			}
		}

	return (-1);  //all in use
	}

// b_open is called by the "user application" to open a file.  This routine is 
// similar to the Linux open function.  	
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.  

b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system

	//*** TODO ***:  Write open function to return your file descriptor
	//				 You may want to allocate the buffer here as well
	//				 But make sure every file has its own buffer

	// TODO: return a negative number if there is an error


	b_io_fd fcb = b_getFCB();
	// printf("file control block: %d\n", fd);
	// output:
	// 		file control block: 0
	//		file control block: 1


	fileInfo* get_fi = GetFileInfo(filename);
	// printf("file name: %s\n", get_fi->fileName);
	// printf("file size: %d\n", get_fi->fileSize);
	// printf("location: %d\n", get_fi->location);
	// file name: DecOfInd.txt
	// file size: 8120 // file actual byte length
	// location: 3 // starting block number
	// file name: CommonSense.txt
	// file size: 1877
	// location: 19

	fcbArray[fcb].fi = get_fi;
	fcbArray[fcb].buffer = malloc(B_CHUNK_SIZE);
	fcbArray[fcb].byteCopied = 0;

	// This is where you are going to want to call GetFileInfo and b_getFCB
	return fcb;
	}

// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to 
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File	
int b_read (b_io_fd fd, char * buffer, int count)
	{
	//*** TODO ***:  
	// Write buffered read function to return the data and # bytes read
	// You must use LBAread and you must buffer the data in B_CHUNK_SIZE byte chunks.
		
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fd].fi == NULL)		//File not open for this descriptor
		{
		return -1;
		}	

	// Your Read code here - the only function you call to get data is LBAread.
	// Track which byte in the buffer you are at, and which block in the file	

	// printf("fd: %d\n", fd);
	// printf("buffer: %s\n", buffer);

	// don't have to input anything into the buffer
	if (count <= 0)
	{
		return count;
	}

	// this if statement will only run when we almost finish input everything into
	// the buffer, and we're meeting this situation if only if the after we add the
	// count number of bytes into the buffer, the buffer total size is larger then
	// total number of blocks we declare for holding the text file
	if (fcbArray[fd].byteCopied + count > fcbArray[fd].fi->fileSize)
	{
		// at this time, since we're at the last time to do the b_read() function
		// it's okay to change the count number become a smaller size to hold the rest
		// of the bytes in the text file
		// otherwise, will have error for our program due to too many bad block request
		count = fcbArray[fd].fi->fileSize - fcbArray[fd].byteCopied;
	}

	// initialized a integer variable to hold the total number of bytes 
	// which is already put into the buffer
	int copied = 0;

	// LBAread() function will read the exact block by following the parameter we set
	// into the function
		LBAread(fcbArray[fd].buffer, 1, 
			fcbArray[fd].fi->location + fcbArray[fd].byteCopied / B_CHUNK_SIZE);

	// since the b_read() function will run multiple times to finish read the whole 
	// text file, which means the each buffer block may already have some bytes(data),
	// when we're trying to input bytes into the buffer, we need to check how many remain
	// bytes we have in this block, and we may separate out count(number of bytes) become
	// two part, first fill out the block which already have some data, then input others
	// to the next block
	if (B_CHUNK_SIZE  - fcbArray[fd].byteCopied % B_CHUNK_SIZE != 0)
	{	
		// check if this block's remain bytes having enough space to hold all count bytes
		// we can do memery copy to copy the total count number of bytes into our buffer
		// since all of the count bytes finish input to the buffer, we can simply return 
		// count and finish this function
		if (count <= B_CHUNK_SIZE  - fcbArray[fd].byteCopied % B_CHUNK_SIZE)
		{
			memcpy(buffer, 
				fcbArray[fd].buffer + fcbArray[fd].byteCopied % B_CHUNK_SIZE, count);
			copied += count;
			fcbArray[fd].byteCopied += count;
			// printf("\n\n----count less than remian size in block----\n\n");
			// printf("1. copied: %d\tcount: %d\n", copied, count);
			return count;
		} else { 
			// else statement runs if only if the count bytes is larger than the remain
			// size of this block, what we can do is that we can separate the count bytes
			// become two part, the first part we can input into this block and fill out of
			// this block, for the rest part of the count, we need to do it outside of the
			// else statement
			memcpy(buffer, fcbArray[fd].buffer + fcbArray[fd].byteCopied % B_CHUNK_SIZE, 
				B_CHUNK_SIZE  - fcbArray[fd].byteCopied % B_CHUNK_SIZE);
			copied += (B_CHUNK_SIZE  - fcbArray[fd].byteCopied % B_CHUNK_SIZE);
			fcbArray[fd].byteCopied += (B_CHUNK_SIZE  - fcbArray[fd].byteCopied % B_CHUNK_SIZE);
			// printf("\n\n----count need separate to become two part----\n\n");
		}
	}

	// after finish fill out the block, we need to check the second part of the count bytes
	// we left
	// we need keep going to LBAread next block and memcpy into the buffer
	// using a while loop to check if the rest of the count is larger than a whole block size
	// if the rest of the count bytes size is larger than a whole block size, which means we 
	// need fill out one block in one time by using the while loop until the rest of count
	// is less than one block size and we can jump output the while loop
	while ((count - copied) >= B_CHUNK_SIZE)
	{
		LBAread (fcbArray[fd].buffer, 1, 
			fcbArray[fd].fi->location + fcbArray[fd].byteCopied / B_CHUNK_SIZE);
		memcpy(buffer, 
			fcbArray[fd].buffer + fcbArray[fd].byteCopied % B_CHUNK_SIZE, B_CHUNK_SIZE);
		fcbArray[fd].byteCopied += B_CHUNK_SIZE;
		copied += B_CHUNK_SIZE;
		// printf("\n\n----copied a whole block in one time----\n\n");
	}

	// it may have some situation if the after we finish input the count bytes into one whole
	// block, and we already finish to input all of count bytes into our buffer and nothing 
	// left in this situation, we need to check if the count size is equal to the number of 
	// bytes we've input into the buffer, then we can finish the function
	if (copied == count)
	{
		// printf("2. copied: %d\tcount: %d\n", copied, count);
		return copied;
	}

	// if going outside of the while loop, this means we only left the last part of the count 
	// need to copy to the buffer
	// since we're already fill out the previous block, we have to use LBAread() function to
	// read the next block 512 bytes data first, then we can use memory copy to copy the rest
	// of data into our buffer
	LBAread(fcbArray[fd].buffer, 1, 
		fcbArray[fd].fi->location + fcbArray[fd].byteCopied / B_CHUNK_SIZE);
	memcpy(buffer, 
		fcbArray[fd].buffer + fcbArray[fd].byteCopied % B_CHUNK_SIZE, count - copied);
	fcbArray[fd].byteCopied += (count - copied);
	copied += (count - copied);
	// printf("----copy the rest of the count left----\n");

	// printf("3. copied: %d\tcount: %d\n", copied, count);
	return copied;
	}
	
// b_close frees and allocated memory and places the file control block back 
// into the unused pool of file control blocks.
int b_close (b_io_fd fd)
	{
	//*** TODO ***:  Release any resources
	free(fcbArray[fd].buffer);
	fcbArray[fd].buffer = NULL;
	}
	

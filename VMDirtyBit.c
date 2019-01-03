/* 
VMDirtyBit.c - Program to implement a Virtual Memory Manager that handles a Dirty Bit
Author - Saumya Saxena 
Date - February 25, 2018
Class - CS543 Operating Systems
Assignment - 3
*/
/* Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

/* Global Variables  */

//Pages filled in the page table
int filledFrameNumber = -1;

//Store physical memory
int physicalMemory[2000][256]; // as size of each page is 256 bytes

//Pages filled in TLB
int filledTLBSize = -1;

//page swap due to dirty bit
int pageSwap = 0; //as total number of pages in pageTable is 256

//No of Page fault
int pageFault = 0;

//No of time page in tlb table matches i.e.TLB hit
int TLBHit = 0;

//Store page number with frame number in TLB data (TLB: Translation lookaside buffer)
struct TLBData
{
    int frameNumber;
    int pageNumber;
    char dirtyBit;
}TLBData[16];

//Page table to store pages
struct pageTable
{
    int pageNumber;
    char dirtyBit;	
}pageTable[256];

/* Function Prototypes */
int checkForPageNumInTLB(int pageNumber);
int getFrameNumber(int pageNumber);
void pushIntoTLB(int pageNumber, char dirtyBit);
int checkForPageNumInPageTableWithDirtBit(int pageNumber, char dirtyBit);
int checkForPageNumInPageTable(int pageNumber);
void pushIntoTable(int pageNumber,char dirtyBit);
void readFromBackingStore(int pageNumber);
void LRUpageReplacement(int pageNumber,char dirtyBit);

/* checkForPageNumInTLB - function to check a page number in the TLB

    The function takes a page number as a parameter and checks if the requested page number is available
    in the TLB or not. If yes, returns the corresponding index.
*/
int checkForPageNumInTLB(int pageNumber)
{
    int index = -1;
    for(int i=0;i<=filledTLBSize;i++)
    {
        if(pageNumber == TLBData[i].pageNumber)
        {
            index =i;
            break;
        }
    }
    return index;
}

/* getFrameNumber - function to get frame number

    The function takes a page number as a parameter and returns the corresponding frame number.
*/
int getFrameNumber(int pageNumber)
{
    for(int i=0;i<=filledFrameNumber;i++)
    {
        if(pageNumber == pageTable[i].pageNumber)
        {
            return i;
        }
    }
}

/* pushIntoTLB - function to insert a page into the TLB

    The function takes a page number as a parameter and pushes a new entry into the TLB whenever a 
    replacement occurs or a page reference is made that is in the page table but not in the TLB.
*/
void pushIntoTLB(int pageNumber, char dirtyBit)
{
    int index = -1;
    int indexWithDirtyBit = -1;
    index = checkForPageNumInTLB(pageNumber);
    int frameNumber = getFrameNumber(pageNumber);
    if(index == -1) //Page number is not avaiable in TLBdata
    {
        if(filledTLBSize == 15)
        {
            for(int i=0;i<filledTLBSize;i++)
            {
                TLBData[i].frameNumber = TLBData[i+1].frameNumber;
                TLBData[i].pageNumber = TLBData[i+1].pageNumber;
	        TLBData[i].dirtyBit = TLBData[i+1].dirtyBit;
            }
            TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
	        TLBData[filledTLBSize].dirtyBit = dirtyBit;
        }
        else
        {
            filledTLBSize++;
            TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
	        TLBData[filledTLBSize].dirtyBit = dirtyBit;
        }
    }
    else
    { 
        if(index!=filledTLBSize)
		{ 
            for(int i = index;i<filledTLBSize;i++)
            {
            	TLBData[i].frameNumber = TLBData[i+1].frameNumber;
            	TLBData[i].pageNumber = TLBData[i+1].pageNumber;
                TLBData[i].dirtyBit = TLBData[i+1].dirtyBit;
            }
            TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
            TLBData[filledTLBSize].dirtyBit =dirtyBit;
		}
		else
		{
	   		TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
            TLBData[filledTLBSize].dirtyBit = 'R';
		}
        
    }
}

/* checkForPageNumInPageTableWithDirtBit - check for a page in the page table with Dirty Bit

*/
int checkForPageNumInPageTableWithDirtBit(int pageNumber, char dirtyBit)
{
    int index = -1;
    for(int i=0;i<=filledFrameNumber;i++)
    {
        if(pageNumber == pageTable[i].pageNumber)
        {
            index =0;
            if(pageTable[i].dirtyBit == 'R' && dirtyBit=='W')
            {
                return -1;
            }
        }
    }
    return index;
}

/* checkForPageNumInPageTable - function to check for a page number in the page table

    The function takes a page number as a parameter and returns the index of the page 
    if it is available in the page table.
*/
int checkForPageNumInPageTable(int pageNumber)
{
    int index = -1;
    for(int i=0;i<=filledFrameNumber;i++)
    {
        if(pageNumber == pageTable[i].pageNumber)
        {
            index =i;
            break;
        }
    }
    return index;
}

/*  pushIntoTable - funtion to insert a page into the page table

    The function takes as a parameter a page number and is called whenever we enter a page which is
    not already in the page table. It is called everytime a page fault occurs and a page is chosen 
    to be replaced and a new page has to be inserted into the page table.
*/
void pushIntoTable(int pageNumber,char dirtyBit)
{
    int index = -1;
    int indexWithDirtyBit = -1;
    index = checkForPageNumInPageTable(pageNumber);
    indexWithDirtyBit = checkForPageNumInPageTableWithDirtBit(pageNumber,dirtyBit);
    if(indexWithDirtyBit == -1)
    {
       pageSwap++;
    }
    if(index == -1)// page number not available in page table
    {
        if(filledFrameNumber == 255)
        {
            for(int i = 0;i<filledFrameNumber;i++)
            {
                pageTable[i].pageNumber = pageTable[i+1].pageNumber;
				pageTable[i].dirtyBit = pageTable[i+1].dirtyBit;
            }
            pageTable[filledFrameNumber].pageNumber = pageNumber;
	    	pageTable[filledFrameNumber].dirtyBit = 'R';
        }
        else
        {
            filledFrameNumber++;
            pageTable[filledFrameNumber].pageNumber = pageNumber;
            pageTable[filledFrameNumber].dirtyBit = 'R';
        }
    }
    else if(index!=-1)
    {
		if(index != filledFrameNumber)
		{
      	    for(int i = index;i<filledFrameNumber;i++)
            {
    		pageTable[i].pageNumber = pageTable[i+1].pageNumber;
	        pageTable[i].dirtyBit = pageTable[i+1].dirtyBit;
       	    }  
       		pageTable[filledFrameNumber].pageNumber = pageNumber;
        	pageTable[filledFrameNumber].dirtyBit = dirtyBit;
        }
		else
		{
	     	pageTable[filledFrameNumber].pageNumber = pageNumber;
            pageTable[filledFrameNumber].dirtyBit = dirtyBit;
		}
   }
}

/*  readFromBackingStore - function to read a physical address from the BACKING STORE corresponding 
    to a given page number

    The function takes as a parameter a page number calculated from the logical address and reads 
    the corresponding frame number to get the actual physical address.
*/
void readFromBackingStore(int pageNumber)
{
    FILE *fBackingStore = fopen("BACKING_STORE.bin","rb");
    char str[256];
    fseek(fBackingStore,pageNumber*256,SEEK_SET);
    fread(str,sizeof(char),256,fBackingStore);
    for(int i=0;i<256;i++)
    {
        physicalMemory[getFrameNumber(pageNumber)][i] = str[i];
    }
}

/* LRUpageReplacement - the page replacement algorithm used

    LRU or Least Recently Used replacement algorithm is implemented using Stack data structure. 
    Whenever a page is requested, it is first searched for in the TLB. If the TLB has it, the 
    corresponding frame number is returned otherwise, if the TLB does not have the requested page, 
    the search moves to the Page Table. If the page table has the requested page, the corresponding 
    frame number is returned and an entry into the TLB is made otherwise page replacement is done by 
    chosing a victim page according to the LRU algorithm.
    For the Stack implementation, when a reference to a page is made, its' corresponding entry is moved to 
    the top of the TLB and when replacing a page, it is replaced from the bottom of the TLB.
*/
void LRUpageReplacement(int pageNumber,char dirtyBit)
{
    if(filledFrameNumber == -1)
    {
        filledFrameNumber++;
        filledTLBSize++;
        pageFault++;
        pageTable[filledFrameNumber].pageNumber = pageNumber;
        pageTable[filledFrameNumber].dirtyBit = 'R';
        TLBData[filledTLBSize].frameNumber = 0;
        TLBData[filledTLBSize].pageNumber = pageNumber;
        TLBData[filledTLBSize].dirtyBit = dirtyBit;
        readFromBackingStore(pageNumber);
    }
    else if(filledFrameNumber <= 255)
    {
        if(checkForPageNumInTLB(pageNumber) != -1)
        {
 	    pushIntoTable(pageNumber,dirtyBit);
            pushIntoTLB(pageNumber,dirtyBit);
	     
            TLBHit ++;
        }
        else if(checkForPageNumInPageTable(pageNumber) != -1)
        { 
	    pushIntoTable(pageNumber,dirtyBit);
            pushIntoTLB(pageNumber,dirtyBit);
        }
        else 
        {  
            pushIntoTable(pageNumber,dirtyBit);
            pushIntoTLB(pageNumber,dirtyBit);
		
	        // Page not found in tlb as well as page table so we pull it from physical memory 
            readFromBackingStore(pageNumber);
            pageFault++;
        }
    }
}

/* main - the main routine of the program */
int main(int argc, char *argv[])
{
    // Addresses file: take addresses2.txt as argument
    FILE *fAdress = fopen(argv[1], "r");

    // logical address
    int logAdd = 0;

    // page number
    int pageNum = 0;

    // dirty bit 
    char dirtyBit;

    // offset
    int offset =0;
    char ch;

    //Total number of addresses in the file
    int numberOfAddresses = 0;

    while(fscanf(fAdress,"%d %c\n", &logAdd,&dirtyBit)==2) {

        numberOfAddresses++;
	    
        // mask the logical address with 16 bits and right shift 8 to obtain page number
        pageNum = logAdd & 65280;
        pageNum = pageNum >> 8;
	
	    //mask the logical address with 8 bits to obtain offset
        offset = logAdd & 255;


        LRUpageReplacement(pageNum,dirtyBit);

        int frameNumber = getFrameNumber(pageNum);
        int value = physicalMemory[getFrameNumber(pageNum)][offset];
        int physicalAddress = (frameNumber <<8 )|offset;
        printf("Logical Address: %d Physical Address: %d Value: %d\n",logAdd,physicalAddress,value);
    }
    printf("\nNumberofAdresses:%d \nPageFault: %d\nPage Swap/Page fault Due to DirtyBit: %d\nTLBHit: %d				 \n",numberOfAddresses,pageFault,pageSwap,TLBHit);
    float TLBHitRate = (float)(TLBHit)/numberOfAddresses;
    float pageFaultRate = (float)pageFault/numberOfAddresses;
    float pageSwapRate = (float)pageSwap/numberOfAddresses;
    printf("TLB hit rate:%f \nPage Fault Rate:%f \nPage Swap Rate Due to Dirty Bit:%f\n",TLBHitRate,pageFaultRate,pageSwapRate);
    return 0;
}



/* 
VMmanager_WithModification.c - Program to implement a Virtual Memory Manager with a smaller physical address space
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
int physicalMemory[2000][128];   // as size of each page is 128 bytes

//Pages filled in TLB
int filledTLBSize = -1;

//Page table to store pages
int pageTable[128];// as total number of pages in pageTable is 128

//No of Page fault
int pageFault = 0;

//No of time page in tlb table matches i.e.TLB hit
int TLBHit = 0;

//Store page number with frame number in TLB data (TLB: Translation lookaside buffer)
struct TLBData
{
    int frameNumber;
    int pageNumber;
}TLBData[16];   // size of tlb is 16 bytes

/* Function Prototypes */
int checkForPageNumInTLB(int pageNumber);
int getFrameNumber(int pageNumber);
void pushIntoTLB(int pageNumber);
int checkForPageNumInPageTable(int pageNumber);
void pushIntoTable(int pageNumber);
void readFromBackingStore(int pageNumber);
void LRUpageReplacement(int pageNumber);

/*  checkForPageNumInTLB - function to check a page number in the TLB

    The function takes a page number as a parameter and checks if the requested page number is 
    available in the TLB or not. If yes, returns the corresponding index else return -1.
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

/*  getFrameNumber - function to get frame number

    The function takes a page number as a parameter and returns the corresponding frame number from 
    page table.
*/
int getFrameNumber(int pageNumber)
{
    for(int i=0;i<=filledFrameNumber;i++)
    {
        if(pageNumber == pageTable[i])
        {
            return i;
        }
    }
}

/* pushIntoTLB - function to insert a page into the TLB

    The function takes a page number as a parameter and pushes a new entry into the TLB whenever a replacement 
    occurs or a page reference is made that is in the page table but not in the TLB.
*/
void pushIntoTLB(int pageNumber)
{
    int index = -1;
    index = checkForPageNumInTLB(pageNumber);
    int frameNumber = getFrameNumber(pageNumber);
    if(index == -1)    //Page number is not avaiable in TLBdata
    {
        if(filledTLBSize == 15)
        {
            for(int i=0;i<filledTLBSize;i++)
            {
                TLBData[i].frameNumber = TLBData[i+1].frameNumber;
                TLBData[i].pageNumber = TLBData[i+1].pageNumber;
            }
            TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
        }
        else
        {
            filledTLBSize++;
            TLBData[filledTLBSize].frameNumber = frameNumber;
            TLBData[filledTLBSize].pageNumber = pageNumber;
        }
    }
    else     //Page number is available in TLB need to move at top of the TLB
    {
        for(int i = index;i<filledTLBSize;i++)
        {
            TLBData[i].frameNumber = TLBData[i+1].frameNumber;
            TLBData[i].pageNumber = TLBData[i+1].pageNumber;
        }
        TLBData[filledTLBSize].frameNumber = frameNumber;
        TLBData[filledTLBSize].pageNumber = pageNumber;
    }
}

/* checkForPageNumInPageTable - function to check for a page number in the page table

The function takes a page number as a parameter and returns the index of the page if it is available in the page table.
*/
int checkForPageNumInPageTable(int pageNumber)
{
    int index = -1;
    for(int i=0;i<=filledFrameNumber;i++)
    {
        if(pageNumber == pageTable[i])
        {
            index =i;
            break;
        }
    }
    return index;
}

/* pushIntoTable - funtion to insert a page into the page table

    The function takes as a parameter a page number and is called whenever we enter a page which is not already in the page table.
    It is called everytime a page fault occurs and a page is chosen to be replaced and a new page has to be inserted into the page table.
*/
void pushIntoTable(int pageNumber)
{
    int index = -1;

    index = checkForPageNumInPageTable(pageNumber);
    if(index == -1)   // page number not available in page table
    {
        if(filledFrameNumber == 127)
        {
            for(int i = 0;i<filledFrameNumber;i++)
            {
                pageTable[i] = pageTable[i+1];
            }
            pageTable[filledFrameNumber] = pageNumber;
        }
        else
        {
            filledFrameNumber++;
            pageTable[filledFrameNumber] = pageNumber;
        }
    }
    else   // page number is available need to move at the top
    {
        for(int i = index;i<filledFrameNumber;i++)
        {
            pageTable[i] = pageTable[i+1];
        }
        pageTable[filledFrameNumber] = pageNumber;
    }
}

/* readFromBackingStore - function to read a physical address from the BACKING STORE corresponding to a given page number

    The function takes as a parameter a page number calculated from the logical address and reads the corresponding frame number to 
    get the actual physical address.
*/
void readFromBackingStore(int pageNumber)
{
    FILE *fBackingStore = fopen("BACKING_STORE.bin","rb");
    char str[128];
    fseek(fBackingStore,pageNumber*128,SEEK_SET);
    fread(str,sizeof(char),128,fBackingStore);
    for(int i=0;i<128;i++)
    {
        physicalMemory[getFrameNumber(pageNumber)][i] = str[i];
    }
}

/* LRUpageReplacement - the page replacement algorithm used

    LRU or Least Recently Used replacement algorithm is implemented using Stack data structure. Whenever a page is requested, it is 
    first searched for in the TLB. If the TLB has it, the corresponding frame number is returned otherwise, if the TLB does not have 
    the requested page, the search moves to the Page Table. If the page table has the requested page, the corresponding frame number 
    is returned and an entry into the TLB is made otherwise page replacement is done by chosing a victim page according to the LRU algorithm.
    For the Stack implementation, when a reference to a page is made, its' corresponding entry is moved to the top of the TLB and when 
    replacing a page, it is replaced from the bottom of the TLB.
*/
void LRUpageReplacement(int pageNumber)
{
    if(filledFrameNumber == -1)  // first page number is inserted in tlb as well as in page table
    {
        filledFrameNumber++;
        filledTLBSize++;
        pageFault++;
        pageTable[filledFrameNumber] = pageNumber;
        TLBData[filledTLBSize].frameNumber = 0;
        TLBData[filledTLBSize].pageNumber = pageNumber;
        readFromBackingStore(pageNumber);
    }
    else if(filledFrameNumber <= 127)
    {
        if(checkForPageNumInTLB(pageNumber) != -1)
        {
            pushIntoTable(pageNumber);
            pushIntoTLB(pageNumber);
            TLBHit ++;
        }
        else if(checkForPageNumInPageTable(pageNumber) != -1)
        {
	        pushIntoTable(pageNumber);
            pushIntoTLB(pageNumber);
        }
        else
        {
            pushIntoTable(pageNumber);
            pushIntoTLB(pageNumber);
		
	        // Page not found in tlb as well as page table so we pull it from physical memory 
            readFromBackingStore(pageNumber);
            pageFault++;
        }
    }
}

/* main - the main routine of the program */
int main(int argc, char *argv[])
{
    // Addresses file : Take addresses.txt as argument 
    FILE *fAdress = fopen(argv[1], "r");
    // logical address
    int logAdd = 0;  

    // page number
    int pageNum = 0; 

    // offset
    int offset =0;   

    //Total number of addresse in the file
    int numberOfAddresses = 0;

    while(fscanf(fAdress, "%d", &logAdd) == 1)   // Read line by line 
    {

        numberOfAddresses++;
	
	    // mask the logical address with 16 bits and right shift 8 to obtain page number
        pageNum = logAdd & 65280;
        pageNum = pageNum >> 8;
	
	    //mask the logical address with 8 bits to obtain offset
        offset = logAdd & 255;


        LRUpageReplacement(pageNum);

        int frameNumber = getFrameNumber(pageNum);
	    if(offset > 127)
	    {
		    offset = offset % 128;   // There is 128 byte space is available in physical memory
	    }

        int value = physicalMemory[getFrameNumber(pageNum)][offset];   // Find the value at the address in physical memory
        int physicalAddress = (frameNumber <<8 )|offset; // physical address

        printf("Logical Address: %d Physical Address: %d Value: %d\n",logAdd,physicalAddress,value);

    }
    printf("\nNumberofAdresses:%d \nPageFault: %d \nTLBHit: %d\n",numberOfAddresses,pageFault,TLBHit);
    float TLBHitRate = (float)(TLBHit)/numberOfAddresses;
    float pageFaultRate = (float)pageFault/numberOfAddresses;

    printf("TLB hit rate:%f \nPage Fault Rate:%f\n",TLBHitRate,pageFaultRate);
    return 0;
}



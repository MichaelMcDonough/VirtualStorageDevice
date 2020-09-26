////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache implementation for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Michael McDonough
//   Last Modified : FRI APRIL 17 2020
//

// Includes 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <lcloud_cache.h>


//
// Functions


//Struct to keep track of each value in the cache line
typedef struct {
    int devid;
    int sector;
    int block;
    int time;
    char data[256];

}cache;

//Create an array of cache lines 
cache lrucache[LC_CACHE_MAXBLOCKS];
//Variable to keep track of the size of the cache
int currentsize;
//Variables to count the amount of hits and misses
int hits, misses;

//Variable to keep track of when variables were accessed
int currenttime = 0;

//Create a pointer for the cache
cache *ptr = &lrucache;

int smallesttime = 0;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_getcache
// Description  : Search the cache for a block 
//
// Inputs       : did - device number of block to find
//                sec - sector number of block to find
//                blk - block number of block to find
// Outputs      : cache block if found (pointer), NULL if not or failure

char *lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk ) {
    

    //Loop through the cache to find the block we want
    for (int i=0; i<currentsize; i++){
        if (lrucache[i].devid == did && lrucache[i].sector == sec && lrucache[i].block == blk){
            //If the specific block exists in the cache, return it's data and update time and hits
            hits++;
            return (lrucache[i].data);
        }
    }
    //If the block doesnt exist in the cache
    misses++;
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_putcache
// Description  : Put a value in the cache 
//
// Inputs       : did - device number of block to insert
//                sec - sector number of block to insert
//                blk - block number of block to insert
// Outputs      : 0 if succesfully inserted, -1 if failure

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block ) {
    //If cache is not full yet
    if (currentsize < LC_CACHE_MAXBLOCKS){
        //Loop through the cache and see if its in there already
        for (int i=0; i<currentsize; i++){
            if (lrucache[i].devid == did && lrucache[i].sector == sec && lrucache[i].block == blk){
                //If the block was already in the cache, repace the data and timestamp
                memcpy(lrucache[i].data, block, 256);
                lrucache[i].time = currenttime;
                currenttime++;

                return (0);
            }
        }
        //If it's not, then just add it to the end
        memcpy(lrucache[currentsize].data, block, 256);
        lrucache[currentsize].devid = did;
        lrucache[currentsize].sector = sec;
        lrucache[currentsize].block = blk;
        lrucache[currentsize].time = currenttime;
        currenttime++;
        currentsize++;
        return(0);
    }
    //If the cache is full
    //Loop through the cache to see if the block is already in
    for (int j=0; j< LC_CACHE_MAXBLOCKS; j++){
        if (lrucache[j].devid == did && lrucache[j].sector == sec && lrucache[j].block == blk){
            
            //If the block was already in the cache, replace the data and timestamp
            memcpy(lrucache[j].data, block, 256);
            lrucache[j].time = currenttime;
            currenttime++;
            return (0);
        }
    }
    //If the block wasnt found in the cache, evict the lru block and replace it with the new one
    
    //Do this by finding the smallest time and replacing it's block
    for (int y=1; y< LC_CACHE_MAXBLOCKS; y++){
        if (lrucache[y].time == smallesttime){
            lrucache[y].devid = did;
            lrucache[y].sector = sec;
            lrucache[y].block = blk;
            smallesttime++;
            lrucache[y].time = currenttime;
            memcpy(lrucache[y].data, block, 256);
            currenttime++;
            return (0);
        }
    }
    
    
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_initcache
// Description  : Initialze the cache by setting up metadata a cache elements.
//
// Inputs       : maxblocks - the max number number of blocks 
// Outputs      : 0 if successful, -1 if failure

int lcloud_initcache( int maxblocks ) {
    //For each line of the cache, initialize the values to null
    for (int i=0; i<maxblocks; i++){
        currentsize = 0;
        lrucache[i].devid = NULL;
        lrucache[i].sector = NULL;
        lrucache[i].block = NULL;
        lrucache[i].time = NULL;
    }

    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_closecache
// Description  : Clean up the cache when program is closing
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int lcloud_closecache( void ) {
    //Variable for total accesses
    int totaccess;
    float hitratio;
    //Add up total accesses
    totaccess = hits + misses;
    float newhits = (float)hits;
    float newaccess = (float)totaccess;
    //Calculate the hit ratio percentage
    hitratio = (newhits/newaccess);
    //Print out all of the statistics
    logMessage(LcDriverLLevel,
     "\nTOTAL ACCESSES: %d\nTOTAL HITS: %d\nTOTAL MISSES: %d\nHIT RATIO PERCENTAGE: %.2f",
     totaccess, hits, misses, (100.00*hitratio));

    /* Return successfully */
    return( 0 );
}

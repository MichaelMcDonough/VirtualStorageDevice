////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : Michael C McDonough
//   Last Modified : FRI APRIL 17 2020
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>
#include <lcloud_cache.h>
#include <lcloud_support.h>

// Function Prototypes for each of the supplementary functions
LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2 , 
uint64_t d0, uint64_t d1);

void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *b0, uint64_t *b1, uint64_t *c0, uint64_t *c1, uint64_t *c2 
, uint64_t *d0, uint64_t *d1);

int getDeviceID(uint64_t d0);

int *writeblock(int devid, char *buf,  int sector, int block);

int *readblock(int devid, char *buf, int sector, int block);

int deviceInit();

//Declare global variables that hold the value of each register
uint64_t b0, b1, c0, c1, c2, d0, d1;

//Variable to count how many devices we have
int devicecount;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : create_lcloud_registers
// Description  : Takes seven indvidual registers and packs them into a single frame
//
// Inputs       : b0 - the first register (4 bits)
//                b1 - the second register (4 bits)
//                c0 - the third register (8 bits)
//                c1 - the fourth register (8 bits) 
//                c2 - the fifth register (8 bits) 
//                d0 - the sixth register (16 bits)
//                d1 - the seventh register (16 bits)                
//
// Outputs      : VOID
LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2 
, uint64_t d0, uint64_t d1){
    
    //Create local values for each reguster and the return value
    int64_t retval = 0x0, tempb0, tempb1, tempc0, tempc1, tempc2, tempd0, tempd1;
    
    //Pack to the left so b0 is the msb and d1 is lsb
    //Shifting each time so not to overlap the previous register
    tempb0 = (b0&0xF) << 60;
    tempb1 = (b1&0xF) << 56;
    tempc0 = (c0&0xFF) << 48;
    tempc1 = (c1&0xFF) << 40;
    tempc2 = (c2&0xFF) << 32;
    tempd0 = (d0&0xFFFF) << 16;
    tempd1 = (d1&0xFFFF);
    
    //Bitwise Or the values to get the final frame.
    retval = tempb0|tempb1|tempc0|tempc1|tempc2|tempd0|tempd1;
    return retval;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract_lcloud_registers
// Description  : Takes a full register frame and finds the value for each individual register
//
// Inputs       : resp - a full register frame
//                b0 - the first register (4 bits)
//                b1 - the second register (4 bits)
//                c0 - the third register (8 bits)
//                c1 - the fourth register (8 bits) 
//                c2 - the fifth register (8 bits) 
//                d0 - the sixth register (16 bits)
//                d1 - the seventh register (16 bits)
//
// Outputs      : VOID
void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *b0, uint64_t *b1, uint64_t *c0, uint64_t *c1, uint64_t *c2 
, uint64_t *d0, uint64_t *d1){
    

    //To get an individual register, shift all of it over until the end of the desired register is the last bit.
    //Anding it with 0 will remove excess
    //MSB
    *b0 = (resp>>60)&0xF;    
    *b1 = (resp>>56)&0xF;      
    *c0 = (resp>>48)&0xFF;
    *c1 = (resp>>40)&0xFF;
    *c2 = (resp>>32)&0xFF;
    *d0 = (resp>>16)&0xFFFF;
    *d1 = (resp)&0xFFFF;
    //LSB

    //Since each register is global, need not return anything.
}


//Declare a struct to be used to keep track of all information regarding to a specific file
typedef struct {
    char filename;
    char pathname;
    int length;
    size_t pos;
    LcFHandle fhandle;
    int size;
    int open;
    int blocklist[1000];
    int devicelist[1000];
    int sectorlist[1000];
    int writepos[1000];
    int startwrite[1000];
    int totalwritten[1000];
    int writeamount[1000];
    int writecount;
    int offset;
    int newblk;
}file;

//Create an array of the file structs
file instancearray[1000];

//Declare a struct to be used to keep track of all information regarding to a specific device
typedef struct {
    int id;
    int blocks;
    int sectors;
    int powerOn;
    int blocknum;
    int secnum;
    int full;
    int pos;
    int emptyamount;
    int emptyblk[1000];
    int emptysec[1000];
    
}device;


//Create an array of device structs
device devicearray[100];

//Variable to keep track if power is on or not
int powerOn = 0;

//Counter variable to assign a unique file handle to each file
int file_counter = 0;

//instantiate a unique handle for each file
LcFHandle fh;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure
LcFHandle lcopen( const char *path ) {

    //If this is the first open, power on and find all devices
    if (powerOn == 0){
        powerOn = 1;

    //Power On Devices
    LCloudRegisterFrame frm1 = create_lcloud_registers(0, 0, LC_POWER_ON, 0, 0, 0, 0);
    LCloudRegisterFrame busnum1 = client_lcloud_bus_request(frm1, NULL);

    //Probe Device
    LCloudRegisterFrame frm2 = create_lcloud_registers(0, 0, LC_DEVPROBE, 0, 0, 0, 0);
    LCloudRegisterFrame busnum2 = client_lcloud_bus_request(frm2, NULL);

    //extract values
    extract_lcloud_registers(busnum1, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    extract_lcloud_registers(busnum2, &b0, &b1, &c0, &c1, &c2, &d0, &d1);

    //Organize the device ID's for devInit
    getDeviceID(d0);
    
    
    //Initialize each of the devices
    deviceInit();

    //Initialize the cache
    lcloud_initcache(LC_CACHE_MAXBLOCKS);
    }

    //Iteration Variable
    int i;
    
    //Loop through the File Array and return error if the file is already open
    for (i=0; i<file_counter; i++){
        if (instancearray[i].filename == path){
            return(-1);
        }
    }
    //Create a Struct instance for this file
    instancearray[file_counter].size = 0;
    instancearray[file_counter].filename = *path;
    instancearray[file_counter].fhandle = file_counter;
    instancearray[file_counter].open = 1;
    instancearray[file_counter].newblk = 0;
    fh = instancearray[file_counter].fhandle;
    file_counter++;
    
    // Return File Handle
    return (fh);   
}



 ////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcread
// Description  : Read a specific portion of data from the file 
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure
int lcread( LcFHandle fh, char *buf, size_t len ) {
    //Declare local variables that will be used
    int i, handle, currentcount;

    //loop through to find the file thats been passed
    for (i=0; i< file_counter;i++){
        if (instancearray[i].fhandle == fh && instancearray[i].open == 1){
            //The specific file (f) that is open and passed through has fh of value i.
            handle = i;
            break;
        }
    }

    //Creating a temporary buffer so I can transfer specific portions of a block to the final buffer
    char localbuf[256];

    //Create a temporary length value that we can use to keep track of how much of the total length we have left to read
    size_t templen = len;

    // AmountRead variable is used to keep track of how much is read in total.  It will be returned at the end.
    size_t amountRead = 0;

    //Create a pointer for the file we are using
    file *ptr = &instancearray[handle];

    // If file isnt open yet, return error
    if (instancearray[fh].open == 0){ 
        return -1;
    }

    //If an invalid length is recieved, return error
    if (len < 0){
        return -1;
    }   
    int j = 0;
    //Find the block where the amount in the file is first above the offset
    while(instancearray[handle].totalwritten[j] <= ptr->pos){
        j++;
    }

    
    currentcount = j;
    

    //Difference between how much has been written and our start position
    int startdiff = instancearray[handle].totalwritten[currentcount] - ptr->pos;

    //Find the position in the first block that we start reading
    int startpos = instancearray[handle].writepos[currentcount] - startdiff;
    
    //Variable to keep track of position while reading
    int position = instancearray[handle].startwrite[currentcount]+ startpos;

    //Update file position
    ptr->pos += len;

    //If the length of the read is zero, just return
    if (templen == 0){
        return (len);
    }       
    //Loop through the following cases as long as there is still data to be read
    while(templen >0){
        //If the length of the read is on more than one block
        if (templen >= (instancearray[handle].writepos[currentcount]- (position%256))){
            
            //Try to get the data from cache, if it's not there, read from device and revise cache
           if (lcloud_getcache(instancearray[handle].devicelist[currentcount], 
            instancearray[handle].sectorlist[currentcount], 
            instancearray[handle].blocklist[currentcount]) == NULL){
                readblock(instancearray[handle].devicelist[currentcount], localbuf, instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount]);
                lcloud_putcache(instancearray[handle].devicelist[currentcount],instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount], localbuf);
            }
            else{
                memcpy(localbuf, lcloud_getcache(instancearray[handle].devicelist[currentcount], instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount]), 256);
            }

            //Copy what we need from the local buffer into the final buffer
           
            memcpy(&buf[amountRead], &localbuf[((position) % 256)], instancearray[handle].writepos[currentcount] - (position%256));
            
            //Subtract the amount we just copied from templen, and whatever excess there is, will go back through the while loop
            templen -=(instancearray[handle].writepos[currentcount] - (position%256));
            
            //Add the amount we just transfered to the total read.
            amountRead += instancearray[handle].writepos[currentcount] - (position%256);
            
            //Update our position to the end of the block
            position += (256 - (position % 256));
            
            //Update what file block we are on
            currentcount +=1;
        }

        //If the templen stays on just one block
        if (templen < (instancearray[handle].writepos[currentcount] - (position%256)) && templen > 0){
            
            
            //Try to get the data from cache, if it's not there, read from device and revise cache
            if (lcloud_getcache(instancearray[handle].devicelist[currentcount], 
            instancearray[handle].sectorlist[currentcount], 
            instancearray[handle].blocklist[currentcount]) == NULL){
                readblock(instancearray[handle].devicelist[currentcount], localbuf, instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount]);
                lcloud_putcache(instancearray[handle].devicelist[currentcount],instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount], localbuf);
            }
            else{
                memcpy(localbuf, lcloud_getcache(instancearray[handle].devicelist[currentcount], instancearray[handle].sectorlist[currentcount], instancearray[handle].blocklist[currentcount]), 256);
            }
            
            //Copy what we need from the local buffer into the final buffer 
            memcpy(&buf[amountRead], &localbuf[((position) % 256)], templen);
            
            //Update amountread
            amountRead += templen;
            
            //Templen is now zero
            templen = 0;
            
            //Update our position to the end of the block
            position += (256 - position % 256);
        }
        //If the templen is zero, we are done reading, return length
        if (templen == 0){
            return(amountRead);
        }
    }
}



//Initialize the sector and block values for the write-availability array 
int sec = 0;
int blk = 0;

//Variable to keep track of device
int devcount;

//We only want to malloc once so if it is, this variable turns to 1
int maldone =0;
//Declare the dynamic memory array
char *memarr;

//Variable to keep track of empty and refilled block
int totalempty =0;
int refilled = 0;
int newblk = 0;
int fullcount = 0;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//
// Inputs       : dev - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure
int lcwrite( LcFHandle fh, char *buf, size_t len ) {
    //Variables for loop counters
    int i, f, blocks, p, z, emptydev, emptyblock, emptysector;

    //If there is a partially full block found, this will be 1 
    int foundpartial = 0;
    
    
    
    //If this is the first write, allocate the dynamic memory, if not, then set it to spaces
    if (maldone ==0){
        maldone = 1;
        memarr = (char*)malloc(256*sizeof(char));
    }
    else{
        memset(memarr," ", 256);
    }

    //loop through to find the file thats been passed
    for (i=0; i< file_counter;i++){
        if (instancearray[i].fhandle == fh && instancearray[i].open == 1){
            //The specific file (f) that is open and passed through has fh of value i.
            f = i;
            break;
        }
    }
    //Create a pointer for the file we are using
    file *ptr = &instancearray[f];


    //Create a temporary length value that we can use to keep track of how much of the total length we have left to write
    size_t templen = len;

    // If file isnt open yet, return error
    if (ptr->open == 0){    
        return -1;
    }

    //Creating a temporary buffer so I can transfer specific portions of a written peice to be the final result
    char locbuf[256];
    
    //Counter to transfer from buf to dynamic memory
    int transfer = 0;

    //If the length is zero return the length
    if (templen <= 0){
        return(len);
    }

    while (templen > 0){
        foundpartial = 0;
        emptyblock = -1;
        emptydev = -1;
        emptysector = -1;
        z = 0;
        int full = 0;

        //Loop through all the blocks that we've written this file to
        //  Look for one that is partially full
        while (instancearray[f].writepos[z] != 0){
            
            if(instancearray[f].writepos[z] < 256){               
                p = z;
                foundpartial = 1;
                break;
            }
            z++;
        }
        

            //If we find one that is partially full
            if (foundpartial == 1){

                //if for some reason the block we find is outside of the alloted space, skip this entire process
                for(int d = 0; d < devicecount;d++){
                    if (devicearray[d].id == instancearray[f].devicelist[p]){
                        if (devicearray[d].blocks == instancearray[f].blocklist[p] || devicearray[d].sectors == instancearray[f].sectorlist[p]){
                            foundpartial = 0;
                            full = 1;
                        }
                    }
                }
                //If we go past the end of the block
                if (templen > (256 - instancearray[f].writepos[p]) && full == 0){

                    //Read whats already in the block and copy it to local buffer
                    readblock(instancearray[f].devicelist[p], locbuf, instancearray[f].sectorlist[p], instancearray[f].blocklist[p]);
                   

                    memcpy(&locbuf[instancearray[f].writepos[p]], &buf[transfer], (256 - instancearray[f].writepos[p]));
                    transfer += (256 - instancearray[f].writepos[p]);

                    //Now that we know where to write to, we physically put the block into the cache and device memory
                    lcloud_putcache(instancearray[f].devicelist[p], instancearray[f].sectorlist[p], instancearray[f].blocklist[p], locbuf);
                    writeblock(instancearray[f].devicelist[p], locbuf, instancearray[f].sectorlist[p], instancearray[f].blocklist[p]);
                    //memset(locbuf, 0, 256);

                    //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                    // in the next block
                    templen -= (256 - instancearray[f].writepos[p]);

                    //Update total written and the written position
                    instancearray[f].totalwritten[p] +=(256 - instancearray[f].writepos[p]);

                    //keep track of the read/head and length of the file
                    ptr->pos += (256 - instancearray[f].writepos[p]);
                    ptr->length += (256 - instancearray[f].writepos[p]);

                    //Update the original block write length
                    instancearray[f].writepos[p] = 256;

                    
            
                }

                //If we stay inside that block
                if (templen <= (256 - instancearray[f].writepos[p]) && full == 0){
                    
                    //Read whats already in the block and copy it to local buffer
                    readblock(instancearray[f].devicelist[p], locbuf, instancearray[f].sectorlist[p], instancearray[f].blocklist[p]);
                    
                    
                    memcpy(&locbuf[instancearray[f].writepos[p]], &buf[transfer],templen);
                    transfer += templen;


                    //Now that we know where to write to, we physically put the block into the cache and device memory
                    lcloud_putcache(instancearray[f].devicelist[p], instancearray[f].sectorlist[p], instancearray[f].blocklist[p], locbuf);
                    writeblock(instancearray[f].devicelist[p], locbuf, instancearray[f].sectorlist[p], instancearray[f].blocklist[p]);
                    //memset(locbuf, 0, 256);

                    //Update total written in block 
                    instancearray[f].writepos[p] += templen;

                    //Update total written
                    instancearray[f].totalwritten[p] +=templen;

                    //keep track of the read/head and length of the file
                    ptr->pos += templen;
                    ptr->length += templen;

                    //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                    // in the next block
                    templen = 0;

                }
            }
            
            
        

            //Starting at the beginning of a new block and Going past the end of the block
            if (templen >= 256 && foundpartial == 0){
                //memset(locbuf, '\0', 256);
                
                //Find how many blocks this is writing to
                blocks = (int)(templen/256);
                
                //Write to each necessary block
                for (int x=0; x< blocks; x++){
                    
                   memcpy(locbuf, &buf[len - templen], 256);

                    //Look for empty blocks that are due to file closing
                    for(int d = 0; d < devicecount; d++){
                        if (devicearray[d].emptyamount >0){
                            for(int e = 1; e < devicearray[d].emptyamount;e++){
                                //Find the first empty block
                                emptyblock = devicearray[d].emptyblk[e-1];
                                emptysector = devicearray[d].emptysec[e-1];
                                emptydev = devicearray[d].id;

                                //Remove this block from the empty array
                                for(int c = (e-1); c<(devicearray[d].emptyamount-1); c++){
                                    devicearray[d].emptyblk[c] = devicearray[d].emptyblk[c+1];
                                }

                                //Update how many blocks are empty
                                devicearray[d].emptyamount -=1;
                                totalempty -= 1;
                                refilled += 1;

                                break;
                            }
                            break;
                        }
                    }
                    
                    //If an empty block was found, use this
                    if (emptyblock != -1 && emptydev != -1 && emptysector != -1){

                    //Now that we know where to write to, we physically put the block into the cache and device memory
                    lcloud_putcache(emptydev, emptysector, emptyblock, locbuf);
                    writeblock(emptydev, locbuf, emptysector, emptyblock);
                    //memset(locbuf, 0, 256);


                    //Record where we wrote for read functionality
                    instancearray[f].startwrite[instancearray[f].writecount] = 0;
                    instancearray[f].writepos[instancearray[f].writecount] = 256;
                    instancearray[f].writeamount[instancearray[f].writecount] = 256;
                    instancearray[f].devicelist[instancearray[f].writecount] = emptydev;
                    instancearray[f].sectorlist[instancearray[f].writecount] = emptysector;
                    instancearray[f].blocklist[instancearray[f].writecount] = emptyblock;

                    //If this is the first write, the total written is just the 256
                    // If not the first write, it is the 256 added to the previous totalwritten
                    if (instancearray[f].writecount == 0){
                        instancearray[f].totalwritten[instancearray[f].writecount] =256;
                    }
                    else{
                        instancearray[f].totalwritten[instancearray[f].writecount] =256 + instancearray[f].totalwritten[instancearray[f].writecount -1];
                    }
                    
                    //Update how many times we have written from this file
                    instancearray[f].writecount ++;

                    //keep track of the read/head and length of the file
                    ptr->pos += (size_t) 256;
                    ptr->length += (size_t) 256;
                    

                    //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                    // in the next block
                    templen -= (size_t) 256;
                    
                    }

                    //If no empty block was found, proceed as normal
                    else{

                    //Check to make sure device isnt full
                        for (int y = 0; y<devicecount; y++){
                            if (devicearray[devcount].full ==1){
                                devcount += 1;
                                
                                if (devcount >=devicecount){
                                    devcount = 0;
                                }
                            }
                            else{
                                break;
                            }
                        }

                   
                        //Record where we wrote for read functionality
                    instancearray[f].startwrite[instancearray[f].writecount] = 0;
                    instancearray[f].writepos[instancearray[f].writecount] = 256;
                    instancearray[f].writeamount[instancearray[f].writecount] = 256;
                    instancearray[f].devicelist[instancearray[f].writecount] = devicearray[devcount].id;
                    instancearray[f].sectorlist[instancearray[f].writecount] = devicearray[devcount].secnum;
                    instancearray[f].blocklist[instancearray[f].writecount] = devicearray[devcount].blocknum;

                    //Now that we know where to write to, we physically put the block into the cache and device memory
                    lcloud_putcache(devicearray[devcount].id, devicearray[devcount].secnum, devicearray[devcount].blocknum, locbuf);
                    writeblock(devicearray[devcount].id, locbuf, devicearray[devcount].secnum, devicearray[devcount].blocknum);
                    //memset(locbuf, 0, 256);

                    //If this is the first write, the total written is just the 256
                    // If not the first write, it is the 256 added to the previous totalwritten
                    if (instancearray[f].writecount == 0){
                        instancearray[f].totalwritten[instancearray[f].writecount] =256;
                    }
                    else{
                        instancearray[f].totalwritten[instancearray[f].writecount] =256 + instancearray[f].totalwritten[instancearray[f].writecount -1];
                    }
                    
                    //Update how many times we have written from this file
                    instancearray[f].writecount ++;

                    //keep track of the read/head and length of the file
                    ptr->pos += (size_t) 256;
                    ptr->length += (size_t) 256;
                    

                    //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                    // in the next block
                    templen -= (size_t) 256;

                    //Update the block we are on
                    devicearray[devcount].blocknum += 1;

                    //Check to make sure were not going past available space
                    if (devicearray[devcount].blocknum >= devicearray[devcount].blocks){
                        devicearray[devcount].secnum += 1;
                        devicearray[devcount].blocknum = 0;
                    }
                    if (devicearray[devcount].secnum >= devicearray[devcount].sectors){
                        devicearray[devcount].full = 1;
                    }
                    //Go to the next device for the upcoming write
                    devcount+=1;
                    //Since we only have 5 devices, go back to the first after the fifth
                    if (devcount >=devicecount){
                        devcount = 0;
                    }
                    
                    }
                }
            }
            //Starting at the beginning of a new block and Going less than to the end of the block
            if (foundpartial == 0 && templen < 256 && templen > 0){
               
               memcpy(locbuf, &buf[len - templen], templen);

                //Look for empty blocks that are due to file closing
                    for(int d = 0; d < devicecount; d++){
                        if (devicearray[d].emptyamount >0){
                            for(int e = 1; e < devicearray[d].emptyamount;e++){
                                //Find the first empty block
                                emptyblock = devicearray[d].emptyblk[e-1];
                                emptysector = devicearray[d].emptysec[e-1];
                                emptydev = devicearray[d].id;

                                //Remove this block from the empty array
                                for(int c = (e-1); c<(devicearray[d].emptyamount-1); c++){
                                    devicearray[d].emptyblk[c] = devicearray[d].emptyblk[c+1];
                                }

                                //Update how many blocks are empty
                                devicearray[d].emptyamount -=1;
                                totalempty -=1;
                                refilled +=1;

                                break;
                            }
                            break;
                        }
                    }
                    
                    //If an empty block was found, use this
                    if (emptyblock != -1 && emptydev != -1 && emptysector != -1){
                        
                        //Now that we know where to write to, we put the block into the cache and the device
                        lcloud_putcache(emptydev, emptysector, emptyblock, locbuf);
                        writeblock(emptydev, locbuf, emptysector, emptyblock);
                        
                        //Update all of the file information for the write
                        instancearray[f].startwrite[instancearray[f].writecount] = 0;
                        instancearray[f].writepos[instancearray[f].writecount] = templen;
                        instancearray[f].writeamount[instancearray[f].writecount] = templen;
                        instancearray[f].devicelist[instancearray[f].writecount] = emptydev;
                        instancearray[f].sectorlist[instancearray[f].writecount] = emptysector;
                        instancearray[f].blocklist[instancearray[f].writecount] = emptyblock;

                        //If this is the first write, the total written is just the templen
                        // If not the first write, it is the templen added to the previous totalwritten
                        if ( instancearray[f].writecount== 0){
                            instancearray[f].totalwritten[instancearray[f].writecount] =templen;
                        }
                        else{
                            instancearray[f].totalwritten[instancearray[f].writecount] =templen + instancearray[f].totalwritten[instancearray[f].writecount -1];
                        }
                        //Update how many times we have written from this file
                        instancearray[f].writecount++;
                        //keep track of the read/head and length of the file
                        ptr->pos += (size_t) templen;
                
                        ptr->length += (size_t) templen;

                        //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                        // in the next block
                        templen -= templen;

                        transfer = 0;
                    }
                    else{

                    //Check if device is full
                    for (int y = 0; y<devicecount; y++){
                        if (devicearray[devcount].full ==1){
                            devcount += 1;
                            if (devcount >=devicecount){
                                devcount = 0;
                            }
                        }
                        else{
                            break;
                        }
                    }
                   
                    //Update all of the file information for the write
                instancearray[f].startwrite[instancearray[f].writecount] = 0;
                instancearray[f].writepos[instancearray[f].writecount] = templen;
                instancearray[f].writeamount[instancearray[f].writecount] = templen;
                instancearray[f].devicelist[instancearray[f].writecount] = devicearray[devcount].id;
                instancearray[f].sectorlist[instancearray[f].writecount] = devicearray[devcount].secnum;
                instancearray[f].blocklist[instancearray[f].writecount] = devicearray[devcount].blocknum;

                //Now that we know where to write to, we put the block into the cache and the device
                lcloud_putcache(devicearray[devcount].id, devicearray[devcount].secnum, devicearray[devcount].blocknum, locbuf);
                writeblock(devicearray[devcount].id, locbuf, devicearray[devcount].secnum, devicearray[devcount].blocknum);
                
                //If this is the first write, the total written is just the templen
                // If not the first write, it is the templen added to the previous totalwritten
                if ( instancearray[f].writecount== 0){
                        instancearray[f].totalwritten[instancearray[f].writecount] =templen;
                    }
                    else{
                        instancearray[f].totalwritten[instancearray[f].writecount] =templen + instancearray[f].totalwritten[instancearray[f].writecount -1];
                    }
                //Update how many times we have written from this file
                instancearray[f].writecount++;
                //keep track of the read/head and length of the file
                ptr->pos += (size_t) templen;
                
                ptr->length += (size_t) templen;

                //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                // in the next block
                templen -= templen;
            
                //Update the block we are on
                devicearray[devcount].blocknum += 1;

                //Check to make sure were not going past available space
                if (devicearray[devcount].blocknum >= devicearray[devcount].blocks){
                    devicearray[devcount].secnum += 1;
                    devicearray[devcount].blocknum = 0;
                }
                if (devicearray[devcount].secnum >= devicearray[devcount].sectors){
                    devicearray[devcount].full = 1;
                }
                //Go to the next device for the upcoming write
                    devcount+=1;
                //Since we only have 5 devices, go back to the first after the fifth
                if (devcount >=devicecount){
                        devcount = 0;
                    }
                transfer = 0;
                }   
                
                    
                
            }
            if (templen == 0){
                return(len);
            }
        }
    return(len);
        
}




////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : position if successful test, -1 if failure
int lcseek( LcFHandle fh, size_t off ) {
    int i, f;
    for (i=0; i< file_counter;i++){
        if (instancearray[i].fhandle == fh && instancearray[i].open == 1){
            //The specific file (f) that is open and passed through has fh of value i.
            f = i;
            break;
        }
    }

    //Create a pointer that can point to the variables of a specific pointer
    file *ptr = &instancearray[f];
   
    //If the offset is greater than the length of the file, return an error
    if (off > ptr->length){
        return -1;
    }

    //Positioning the read/write head at the desired offset.
    ptr->pos = off;
    ptr->offset = off;
     
    return(ptr->offset);
    
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure
int lcclose( LcFHandle fh ) {
    int i,  f, j, dev, sector, block;

    char emptybuf[256];
    memset(emptybuf, "\0", 256);
    for (i=0; i< file_counter;i++){
        if (instancearray[i].fhandle == fh){
            //The specific file (f) that is open and passed through has fh of value i.
            f = i;
            break;
        }
    }


    //If file is closed, return error
    if (instancearray[f].open == 0){
        return -1;
    }
    //If file is open, make it closed
    instancearray[f].open = 0; 
    
    //Clear the memory from the device where that file was opened, since we cannot access it anymore
    for (j=0;j<instancearray[f].writecount;j++){
        writeblock(instancearray[f].devicelist[j],emptybuf, instancearray[f].sectorlist[j], instancearray[f].blocklist[j]);
        
        //Keep track of what block is now free
        dev = instancearray[f].devicelist[j];
        block = instancearray[f].blocklist[j];
        sector = instancearray[f].sectorlist[j];

        //Allow us to rewrite to this block later incase we run out of space
        for(int d = 0; d < devicecount;d++){
            if (devicearray[d].id == dev){
                devicearray[d].emptyamount +=1;
                devicearray[d].emptyblk[devicearray[d].emptyamount-1] = block;
                devicearray[d].emptysec[devicearray[d].emptyamount-1] = sector;
                totalempty += 1;
            }
        }
    }
    
    
    return( 0 );
    
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure
int lcshutdown( void ) {
    //Free the dynamic memory
    free(memarr);

    //Close the cache
    lcloud_closecache();

    //Power off devices
    LCloudRegisterFrame frm = create_lcloud_registers(0,0, LC_POWER_OFF, 0, 0, 0, 0);
    client_lcloud_bus_request(frm, NULL);

    //Close all Files
    for (int i = 0; i <file_counter ; i++){
        if (instancearray[i].open == 1){
            instancearray[i].open = 0;
        }
    }
    return( 0 );
    
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : writeblock
// Description  : physically put what we want into each block of the device
//
// Inputs       : devid - the ID of the device we want to write to
//                buf - the actual value of what we are righting into the block
// Outputs      : 0 if successful, -1 if failure
int *writeblock(int devid, char *buf, int sector, int block){
    int i;
    for (i=0; i<devicecount;i++){
        if (devicearray[i].id == devid){
            break;
        }
    }
    if (sector>= devicearray[i].sectors){
        return(-1);
    }
    //Declare the frame to be used
    LCloudRegisterFrame frm;
    
    //Pack the registers with a write operator and what and where to write
    frm= create_lcloud_registers(0, 0, LC_BLOCK_XFER, devid, LC_XFER_WRITE, sector, block);
    
    //Calling the bus function
    client_lcloud_bus_request(frm, buf);

    return (0);   
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getDeviceID
// Description  : given a number in the form of 2^(x), it will find the value of x,
//                 by shifting binary 1 to the left until they are the same.
//
// Inputs       : d0 - the register that has the number we want to convert.
//                
// Outputs      : the final exponent value, -1 if failure
int getDeviceID(uint64_t d0){
    //Variables to help
    int devcount = 0;
    int shiftcount = 0;
    uint64_t x = 1;

    //Shift the x value over until it lines up with a d0 "1"
    while (x<=d0){
        //Once we get to a one, the shift count is the device id
        if ((d0 & x) !=0){
            devicearray[devcount].id = shiftcount;
            devcount++;
        }
        x = x << 1;
        shiftcount += 1;
    }
    devicecount = devcount;
    return (0);
    
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : readblock
// Description  : extract what is already written in a specific block
//
// Inputs       : devid - the ID of the device we want to write to
//                buf - the actual value of what we are reading from the block
// Outputs      : 0 if successful, -1 if failure
int *readblock(int devid, char *buf, int sector, int block){
    LCloudRegisterFrame frm;
    
    //Pack the registers with a read operator and what and where to read
    frm= create_lcloud_registers(0, 0, LC_BLOCK_XFER, devid, LC_XFER_READ, sector, block);
    //if ( (frm== -1) || ((frm= client_lcloud_bus_request(frm, buf)) == -1) ||(b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER) ) {
    //logMessage( LOG_ERROR_LEVEL, "LC failure writing blkc[%d/%d/%d].", devid, sector, block );
    //return( -1 );
    //}
    //Calling the bus function 
    client_lcloud_bus_request(frm, buf);

    return (0);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : deviceInit
// Description  : Initialize all of the devices
//
// Inputs       : N/A
//
// Outputs      : 0 if successful, -1 if failure
int deviceInit(){
    LCloudRegisterFrame frm, bus;
    int i;
    //Loop through the device id's and find the length of their sectors and blocks
    for (i=0; i<devicecount; i++){
    //Pack the registers with a read operator and what and where to read
    frm = create_lcloud_registers(0, 0, LC_DEVINIT, devicearray[i].id,0, 0, 0);
    
    //Calling the bus function 
    bus =client_lcloud_bus_request(frm, NULL);

    //Assign the blocks and sectors values to their respective places
    extract_lcloud_registers(bus, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    devicearray[i].blocks = d1;
    devicearray[i].sectors = d0;
    }
    return (0);
}
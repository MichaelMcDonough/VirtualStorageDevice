////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : Michael C McDonough
//   Last Modified : 16 March 2020
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>


// Function Prototypes for each of the supplementary functions
LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2 , 
uint64_t d0, uint64_t d1);

void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *b0, uint64_t *b1, uint64_t *c0, uint64_t *c1, uint64_t *c2 
, uint64_t *d0, uint64_t *d1);

int getDeviceID(uint64_t d0);

int *writeblock(int devid, char *buffer);

int *readblock(int devid, char *buf, int sector, int block);

//Declare global variables that hold the value of each register
uint64_t b0, b1, c0, c1, c2, d0, d1;


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
    *d1 = (resp<<48)&0xFFFF;
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
    
}file;


//Create an array of the file structs
file instancearray[1000];

//Variable to keep track of whether the device is on or off (1-on, 0-off)
int power_on = 0;

//Counter variable to assign a unique file handle to each file
int file_counter = 0;

//Variable to keep track of the ID the device
int devid;

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

    //If the specific file is closed open it
    if ((instancearray[file_counter].open) == 1){
        return -1;
    }
    
    //Power On Devices
    LCloudRegisterFrame frm1 = create_lcloud_registers(0, 0, LC_POWER_ON, 0, 0, 0, 0);
    LCloudRegisterFrame busnum1 = lcloud_io_bus(frm1, NULL);
  
    //Keep track of whether the power is on or not
    if (power_on == 0){
        power_on = 1;
    }

    //Probe Device
    LCloudRegisterFrame frm2 = create_lcloud_registers(0, 0, LC_DEVPROBE, 0, 0, 0, 0);
    LCloudRegisterFrame busnum2 = lcloud_io_bus(frm2, NULL);

    //extract values
    extract_lcloud_registers(busnum1, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    extract_lcloud_registers(busnum2, &b0, &b1, &c0, &c1, &c2, &d0, &d1);

    
    //Create instance of Struct
    instancearray[file_counter].size = 0;
    instancearray[file_counter].pos = 0;
    instancearray[file_counter].filename = *path;
    instancearray[file_counter].fhandle = file_counter;
    instancearray[file_counter].open = 1;
    fh = instancearray[file_counter].fhandle;
    devid = getDeviceID(d0);
    
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
    //Declare variables to keep track of the sector and block of the device we are working with
    int block, sector;

    //Creating a temporary buffer so I can transfer specific portions of a block to the final buffer
    char localbuf[256];

    //Create a temporary length value that we can use to keep track of how much of the total length we have left to read
    size_t templen = len;

    // AmountRead variable is used to keep track of how much is read in total.  It will be returned at the end.
    size_t amountRead = 0;

    //Create a pointer for the file we are using
    file *ptr = &instancearray[file_counter];

    // If file isnt open yet, return error
    if (instancearray[fh].open == 0){ 
        return -1;
    }
    //If an invalid length is recieved, return error
    if (len < 0){
        return -1;
    }
    //Find device
    devid = getDeviceID(d0);

    //Loop through the following cases as long as there is still data to be read
    while(templen >=0){

        /*
        If we start reading at the beginning of a block (the position value is evenly divisible by 256) 
         and go past the end of the 256 byte block
        */
        if ((ptr->pos % 256) ==0 && templen >=256){
            
            //Find specific sector and block we are reading from
            block = (int)((ptr->pos) / 256);
            sector = (int)(((ptr->pos) / 256) / 64);

            //Call readblock to exctract the data of the block and put it into localbuf
            readblock(devid, localbuf, sector, block);
            /*
            Copy the entire length of the localbuf to buf after whats already been read.
             If nothing's been read yet, amountRead will be zero and it will just go at the beginning.
            */
            memcpy(&buf[amountRead], localbuf, 256);
            //Since we just read 256 bytes, add that to the amountRead and position, and subtract it from the temporary length
            amountRead += 256;
            ptr->pos += 256;
            templen -= 256;
            
        }

        //If we start reading in the middle of a block(position/256 has a remainder) and go past the end of the 256 byte block
        else if ((ptr->pos % 256) !=0 && templen >= (256 - (ptr->pos % 256))){
            
            //Find specific sector and block we are reading from
            block = (int)((ptr->pos) / 256);
            sector = (int)(((ptr->pos) / 256) / 64);

            //Call readblock to exctract the data of the block and put it into localbuf
            readblock(devid, localbuf, sector, block);

            /*
            Starting at the portion of the block we want (Hence the splicing on localbuf), 
            we copy only enough from localbuf to buf so that we get to the end of the block.
            */
            memcpy(&buf[amountRead], &localbuf[((ptr->pos) % 256)], (256 - ptr->pos % 256));

            //Subtract the amount we just copied from templen, and whatever excess there is, will go back through the while loop
            templen -= (256- (ptr->pos % 256));
            //Add the amount we just transfered to the total read.
            amountRead += (256 - ptr->pos % 256);
            //Update our position to the end of the block
            ptr->pos += (256 - ptr->pos % 256);
        }

        //If we start at the beginning of a block and read less than to the end
        else if ((ptr->pos % 256) ==0 && templen < (256)){
            
            //Find specific sector and block we are reading from
            block = (int)((ptr->pos) / 256);
            sector = (int)(((ptr->pos) / 256) / 64);

            //Call readblock to exctract the data of the block and put it into localbuf
            readblock(devid, localbuf, sector, block);

            //Starting at the beginning of the block, we only copy the amount of our temp length to the final buffer. 
            memcpy(&buf[amountRead], localbuf, templen);

            //Update our position value
            ptr->pos += templen;
            
            //Add what we just read to the total amount and return it
            amountRead += templen;
            return(amountRead);
        }

        //If we start in the middle of a block and read less than to the end
        else if ((ptr->pos % 256) !=0 && templen < (256 - (ptr->pos % 256))){
            
            //Find specific sector and block we are reading from
            block = (int)((ptr->pos) / 256);
            sector = (int)(((ptr->pos) / 256) / 64);

            //Call readblock to exctract the data of the block and put it into localbuf
            readblock(devid, localbuf, sector, block);

            /*
            Starting at the portion of the block we want (Hence the splicing on localbuf), 
            we copy "templength" amount from localbuf to buf.
            */
            memcpy(&buf[amountRead], &localbuf[(ptr->pos % 256)], templen);

            //Update our position value
            ptr->pos += templen;
            
            //Add what we just read to the total amount and return it
            amountRead += templen;
            return(amountRead);
        }
    }
    //If the while loop ends, just return the amount read
    return(amountRead);
}


//Storage system to tell whether a block is full or not
int storage[LC_DEVICE_NUMBER_SECTORS][LC_DEVICE_NUMBER_BLOCKS];

//Initialize the sector and block values for the write-availability array 
int sec = 0;
int blk = 0;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//

// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure
int lcwrite( LcFHandle fh, char *buf, size_t len ) {

    //Create a pointer for the file we are using
    file *ptr = &instancearray[file_counter];

    //Create a temporary length value that we can use to keep track of how much of the total length we have left to write
    size_t templen = len;

    // If file isnt open yet, return error
    if (ptr->open == 0){    
        return -1;
    }

    //Creating a temporary buffer so I can transfer specific portions of a written peice to be the final result
    char locbuf[256];
    
    //Iterate to the next sector once all blocks are full
    for (sec = 0; sec < 10; sec++){

        //Iterate to the next block once each is full
        for (blk = 0; blk < 64; blk++){
            //If we run out of stuff to write, the loop ends and we return the amount we have written
            if (templen <= 0){
                return (len);
            }

            //Starting at the beginning of an empty block and going to or past the end of the block
            if ((ptr->pos % (size_t) 256) == 0 && (templen >= (size_t) 256) && storage[sec][blk] == 0){
                    
                //copy 256 bytes of the passed buffer to the local buffer
                memcpy(locbuf, buf[len - templen], (size_t) 256);

                //call writeblock to physically put the local buffer into the block 
                writeblock(devid, locbuf);

                //keep track of the read/head and length of the file
                ptr->pos += (size_t) 256;
                ptr->length += (size_t) 256;

                //Use a temporary length to keep track of excess buffer that hasnt been written yet, it will be written
                // in the next block
                templen -= (size_t) 256;

                //This block is now completely full and cannot be written to anymore
                storage[sec][blk] = 1;
            }

            //Starting mid block and going to or past the end of the block
            else if ((ptr->pos % (size_t) 256) != 0 && (templen >= ((size_t) 256 - (ptr->pos  % (size_t)256))) && storage[sec][blk] == 0){
                
                //Read what is in the block so far and adding that to the local buffer
                //because the entire block will be overwritten when write is called
                LCloudRegisterFrame frm = create_lcloud_registers(0, 0, LC_BLOCK_XFER, devid, LC_XFER_READ, sec, blk );
                lcloud_io_bus(frm, locbuf);

                //Copy only the length of the remaining space of the block from buffer to the local buffer
                memcpy(&locbuf[(ptr->pos % (size_t)256)], buf, ((size_t) 256 -(ptr->pos % (size_t)256)));
                
                //Physically write what we have in the buffer, into the block
                writeblock(devid, locbuf);

                //Subtract what we've written from templen
                templen -=((size_t) 256 - (ptr->pos % (size_t)256));
                
                //keep track of the read/head and length of the file
                ptr->pos += ((size_t) 256 - (ptr->pos  % (size_t)256));
                ptr->length += ((size_t) 256 - (ptr->pos  % (size_t)256));

                //The current block is now completely full and cannot be written to anymore
                storage[sec][blk] = 1;
            }

            //Starting at the beginning of an empty  block and going less than end of block
            else if ((ptr->pos % (size_t) 256) == 0 && (templen < (size_t) 256) && storage[sec][blk] == 0 ){
                
                //Copy the entire buffer to our final buffer, starting at the the end of the previous loop
                //  of the current write call.  if this was the first loop, it will start at the very beginning.
                memcpy(locbuf, &buf[len - templen], templen);

                //Physically write what we have in the buffer, into the block
                writeblock(devid, locbuf);

                //keep track of the read/head and length of the file
                ptr->pos += templen;
                ptr->length += templen;

                //Since the entire length has been written, we can just finish and return the length.
                return(len);
            }

            //Starting mid block and going less than to the end of the block
            else if ((ptr->pos % (size_t) 256) != 0 && templen < ((size_t) 256 - (ptr->pos % (size_t)256)) && storage[sec][blk] == 0){
                
                //Read what is in the block so far and adding that to the local buffer
                //because the entire block will be overwritten when write is called
                uint64_t frm1 = create_lcloud_registers(0, 0, LC_BLOCK_XFER, devid, LC_XFER_READ, sec, blk );
                lcloud_io_bus(frm1, locbuf);

                //Copy the entire length of the buffer into the final buffer after what is already in it
                memcpy(&locbuf[(ptr->pos % (size_t)256)], buf, templen);
                
                //Physically write what we have in the buffer, into the block
                writeblock(devid, locbuf);

                //keep track of the read/head and length of the file
                ptr->pos += templen;
                ptr->length += templen;
                
                //Since the entire length has been written, we can just finish and return the length.
                return(len);
            }

            //If the block we are looking at is full, we move to the next one
            if (storage[sec][blk] == 1){
            continue;
            } 
        }
    }
}




////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : 0 if successful test, -1 if failure
int lcseek( LcFHandle fh, size_t off ) {
    
    //Create a pointer that can point to the variables of a specific pointer
    file *ptr = &instancearray[file_counter];
   
    //If the offset is greater than the length of the file, return an error
    if (off > ptr->length){
        return -1;
    }

    //Positioning the read/write head at the desired offset.
    ptr->pos = off;
     
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure
int lcclose( LcFHandle fh ) {

    //If file is closed, return error
    if (instancearray[fh].open == 0){
        return -1;
    }
    //If file is open, make it closed
    instancearray[fh].open = 0; 
    
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
    
    //Power off devices
    LCloudRegisterFrame frm = create_lcloud_registers(0,0, LC_POWER_OFF, 0, 0, 0, 0);
    lcloud_io_bus(frm, NULL);

    //Close all Files
    for (int i = 0; i <1000 ; i++){
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
int *writeblock(int devid, char *buf){
    LCloudRegisterFrame frm;

    //Pack the registers with a write operator and what and where to write
    frm= create_lcloud_registers(0, 0, LC_BLOCK_XFER, devid, LC_XFER_WRITE, sec, blk );

    //Calling the bus function
    lcloud_io_bus(frm, buf);

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
 
    //Variable to keep track of how many times we shift.
    int shiftcount = 0;

    //Binary value 1 that will be shifted
    uint64_t x = 1;

    //Shift binary 1 to the left until it matches the binary number we are given. Add to the counter after each shift.
    while ((d0&x) != d0){

        //Shifting
        x = x << 1;

        //Add to counter
        shiftcount += 1;
    }
    //Return how many times we shifted
    return (shiftcount);
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
    
    //Calling the bus function 
    lcloud_io_bus(frm, buf);

    return (0);
}

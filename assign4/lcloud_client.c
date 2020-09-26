////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_client.c
//  Description   : This is the client side of the Lion Clound network
//                  communication protocol.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sat 28 Mar 2020 09:43:05 AM EDT
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>


// Project Include Files
#include <lcloud_network.h>
#include <cmpsc311_log.h>
#include <lcloud_filesys.h>
#include <lcloud_cache.h>
#include <cmpsc311_util.h>
#include <lcloud_support.h>


// Functions

LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf );

void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *b0, uint64_t *b1, uint64_t *c0, uint64_t *c1, uint64_t *c2 
, uint64_t *d0, uint64_t *d1);

//Variable to know if an open connection was made
int socket_handle = -1;

//Declare global variables that hold the value of each register
uint64_t b0, b1, c0, c1, c2, d0, d1;

int socket_fd;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : This the client regstateeration that sends a request to the 
//                lion client server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed
LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf ) {
    
    uint64_t transfer;
    struct sockaddr_in caddr;
    char *ip = LCLOUD_DEFAULT_IP;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(LCLOUD_DEFAULT_PORT); 

    if (socket_handle == -1){
        socket_handle = 0;

        //Set up address 
        if( inet_aton(ip, &caddr.sin_addr) == 0){
            return( -1);
        } 

        //Create socket
        socket_fd = socket(PF_INET, SOCK_STREAM, 0); 
        if(socket_fd == -1){
            printf( "Error on socket creation [%s]\n", strerror(errno) );
            return( -1);
        }

        //Create connection
        if( connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 ){ 
            printf( "Error on socket connect [%s]\n", strerror(errno) );
            return( -1 );
        }

    }
        // Use the helper function you created in assignment #2 to extract the
        // opcode from the provided register 'reg'
        extract_lcloud_registers(reg, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
        

        // CASE 1s: read operation (look at the c0 and c2 fields)
        // SEND: (reg) <- Network format : send the register reg to the network
        // after converting the register to 'network format'.
        //
        // RECEIVE: (reg) -> Host format
        //          256-byte block (Data read from that frame)
        if (c0 == LC_BLOCK_XFER && c2 == LC_XFER_READ){
            
            //Creating a Network format version of buf
            transfer = htonll64(reg);
            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            } 
            


            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            }   

            
            //Recieve the read block from the server
            if( read( socket_fd, buf, 256) != 256 ) { 
                printf( "Error reading buf [%s]\n", strerror(errno) );
                return( -1);
            } 
           
           
        }


        // CASE 2: write operation (look at the c0 and c2 fields)
        // SEND: (reg) <- Network format : send the register reg to the network
        // after converting the register to 'network format'.
        //       buf 256-byte block (Data to write to that frame)
        //
        // RECEIVE: (reg) -> Host format
        if (c0 == LC_BLOCK_XFER && c2 == LC_XFER_WRITE){
            


            //Creating a Network format version of buf
            transfer = htonll64(reg);

            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            }   
            
            //Sending buf to the server
            if( write( socket_fd, buf, 256) != 256 ) {
                printf( "Error writing buf [%s]\n", strerror(errno) );
                return( -1);
            }
           

            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            }   


        }


        // CASE 3: power off operation
        // SEND: (reg) <- Network format : send the register reg to the network
        // after converting the register to 'network format'.
        //
        // RECEIVE: (reg) -> Host format

        if (c0 == LC_POWER_OFF && c2 == 0){
            printf("POWER OFF\n");


            //Creating a Network format version of buf
            transfer = htonll64(reg);

            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            }   
           
            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            
            }   
            //Convert back to host format
            transfer = ntohll64(transfer);
            printf( "Receivd a value of [%ld]\n", transfer ); 

            // Close the socket when finished : reset socket_handle to initial value of -1.
            // close(socket_handle)
            socket_handle = -1;
            close(socket_fd);

        }

        //Case 4
        //If we are probing for devices
        if (c0 == LC_DEVPROBE && c2 == 0){
            printf("DEVPROBE\n");


            //Creating a Network format version of buf
            transfer = htonll64(reg);

            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            }   
            printf("Sent a value of [%ld]\n", ntohll64(transfer) );

            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            }   
            //Convert back to host format
            transfer = ntohll64(transfer);
            printf( "Receivd a value of [%ld]\n", transfer ); 

        }

        //Case 5
        //If we are initializing devices
        if (c0 == LC_DEVINIT){
            printf("DEVINIT\n");
            //Creating a Network format version of buf
            transfer = htonll64(reg);
            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            }   
            printf("Sent a value of [%ld]\n", ntohll64(transfer) );

            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(transfer) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            
            }   
            //Convert back to host format
            transfer = ntohll64(transfer);
            printf( "Receivd a value of [%ld]\n", transfer ); 
        }

        //Case 6
        //We are powering on devices
        if (c0 == LC_POWER_ON && c2 == 0){
            printf("POWER ON\n");
            //Creating a Network format version of buf
            transfer = htonll64(reg);
            //Sending the value to the server
            if( write( socket_fd, &transfer, sizeof(transfer)) != sizeof(reg) ) {
                printf( "Error writing network data [%s]\n", strerror(errno) );
                return( -1);
            }   
            printf("Sent a value of [%ld]\n", ntohll64(transfer) );

            //Recieve the value back from the server
            if( read( socket_fd, &transfer, sizeof(transfer)) != sizeof(reg) ) { 
                printf( "Error reading network data [%s]\n", strerror(errno) );
                return( -1);
            }
            
            //Convert back to host format
            transfer = ntohll64(transfer);
            printf( "Receivd a value of [%ld]\n", transfer ); 
        }
        return (transfer);
}



////////////////////////////////////////////////////////////////////////////////
//
//  File           : cmpsc311-f16-assign1.c
//  Description    : This is the main source code for for the first assignment
//                   of CMPSC311, Spring 2020, at Penn State.
//                   assignment page for details.
//
//   Author        : Michael McDonough (mcm6061)
//   Last Modified : 5 February 2020
//

// Include Files
#include <stdio.h>
#include <string.h>

#define NUM_TEST_ELEMENTS 100

// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : round311
// Description  : Function to round each float to the nearest integer
//
// Inputs       : num - the value to be rounded
//                
// Outputs      : N/A
int round311(float num) {
    if (num < 0){   // Because casting to int brings it to the floor int, subtract 0.5 for negatives
        return (int) (num - 0.5);
    }
    else{   // add 0.5 for positive numbers
        return (int) (num + 0.5);
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : printfloatArr311
// Description  : Function to print an array of Float values on a single line
//
// Inputs       : array[] - the array of values that will be printed
//                length  - the number of elements in array thats passed
//                
// Outputs      : Prints the values of the array, seperated by a comma
void printfloatArr311 (float array[], int length) {
    for (int i = 0; i < length; ++i)
        if (i == (length-1)) {      //If last element, print number w/out comma after
            printf("%0.2f", array[i]);
        }
        else{   //If not last element, print number followed by comma
            printf("%0.2f" ",", array[i]);          
        }
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : printIntArr311
// Description  : Function to print an array of integers on a single line
//
// Inputs       : array[] - the array of values that will be printed
//                length  - the number of elements in array thats passed
//                
// Outputs      : Prints the values of the array, seperated by a comma
void printIntArr311 (int array[], int length) {
    for (int i = 0; i < length; ++i)
        if (i == (length-1)) {      //If last element, print number w/out comma after
            printf("%i", array[i]);
        }
        else{   //If not last element, print number followed by comma
            printf("%i" ",", array[i]);  
        }
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : bubbleSort311
// Description  : Function to sort an array of integers from least to greatest
//
// Inputs       : array[] - the array of values that will be sorted
//                length  - the number of elements in array thats passed
//                
// Outputs      : N/A
void bubbleSort311 (int array[], int length){
    int i, j, temp;
    for (i = 0; i < length-1; i++) //loop through array as many times as there are elements
        for (j = 0; j < length-i-1; j++)  //Loop again but this time the range is "i" less because for each iteration, the last value is fixed.
            if (array[j] > array[j+1]){     //If the first value is bigger than the second, we swap them
                temp = array[j+1];
                array[j+1] = array[j];
                array[j] = temp;  
            }
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : toBinary
// Description  : Function to take a base 10 integer and convert it to binary
//
// Inputs       : number - the base 10 number wished to convert
//                
// Outputs      : prints the final binary value
void toBinary(int number){
    int binrem[32], i;
    i = 0;
    while (number > 0){ // Keep dividing until the number is 0
        binrem[i] = number % 2; //Divide by two and store remainder in final array
        number /= 2;
        i++;
    }
    while (i % 4 != 0){ // If the loop only printed 3 bits, add another 0 to make it a nibble
        i++;
        binrem[i] = 0;
    }
    for (int j = i - 1; j >= 0; j--){ //Loop through final binary array in reverse and print each element
        printf ("%d", binrem[j]);

        if (j % 4 == 0){    //Print a space every fourth character
        printf(" ");
        }    
    }
    printf("\n");   // Move to new line after every other nibble
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : countRange311
// Description  : Counts the number of elements in an array within a certain range
//
// Inputs       : array[] - the array of values that will be evaluated
//                length  - the number of elements in array thats passed
//                min     - the lower limit of desired range
//                max     - the upper limit of desired range
//                
// Outputs      : count - the amount of elements within range 
int countRange311(float array[], int length, int min, int max){
    int i, new_arr[100], count;
    
    for (i=0; i<length; i++){   //Round the float values into ints
        new_arr[i] = round311(array[i]);   
    }
    
    count = 0;
    for (i=0; i<length; i++){   //keep count of how many elements are between the min and max
        if (new_arr[i] >= min && new_arr[i] <= max){
            count++;
        }
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : histogram311
// Description  : Creates a histogram of the counts of different values in an array
//
// Inputs       : array[] - the array of values that will be used in histogram
//                
// Outputs      : 0 if successful test, -1 if array length is not 10.
int histogram311(int array[]){
    int x, i, j, new_arr[15];
    for (x=0; x<=15; x++){
        new_arr[x] = array[x];

    }
    
    
    if (new_arr[11] <0){
        return -1;
    }
    

    printf("\n   +------------------------------------------+\n");
    for (i=19; i>=0; i--){  //Decrement from 19 to 0 to scale y-axis

        //Create Display of Y-axis
        if (i >=10){    
            if (i == 15 || i == 10 || i == 5 || i == 0){  //add dots for aesthetic reasons
                printf("%i |..", i);
            }
            else{
            printf("%i |  ", i);    
            }
        }

        if (i<10){
            if (i == 15 || i == 10 || i == 5 || i == 0){  //add dots for aesthetic reasons
                printf("0%i |..", i);
            }
            else{
            printf("0%i |  ", i);
            }
        }

        /*iterate through array, see how value compares to the value on y axis:
        If the value is >= to the y value, place an xx, if not add spaces instead
        */
        for (j=0; j<10; j++){
            if (array[j] >= i){
                if (i == 15 || i == 10 || i == 5 || i == 0){ //add dots for aesthetic reasons
                    printf("xx..");
                }
                else{
                    printf("xx  ");
                }
                
            }
            else{
                if (i == 15 || i ==10 || i == 5 || i == 0){   //add dots for aesthetic reasons
                    printf("....");
                }
                else{
                printf("    ");
                }

            }
        } 
        printf("|\n");
    }   //Print x-axis
    printf("   +------------------------------------------+\n");
    printf("      00  10  20  30  40  50  60  70  80  90");
    return 0;
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the CMPSC311 assignment #1
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main(int argc, char **argv) {

    /* Local variables */
    float f_array[NUM_TEST_ELEMENTS];
    int i_array[NUM_TEST_ELEMENTS], i;
    int hist_array[10];


    /* Preamble inforamtion */
    printf( "CMPSC311 - Assignment #1 - Spring 2020\n\n" );

    /* Step #1 - read in the float numbers to process */
    for (i=0; i<NUM_TEST_ELEMENTS; i++) {
        scanf("%f", &f_array[i]);
    }

    /* Step #2 - Alter the values of the float array as follows: all 
    even numbered indices of the array should be multiplied by 0.78 
    if the value is greater than 50 and 1.04 otherwise.  All odd valued
    indices should multiplied by 0.92 if the value is greater than 50 
    and 1.15 otherwise */
    for (i=0; i<NUM_TEST_ELEMENTS; i++) {
        if (i % 2 == 0){    //If number is evenly divisible by 2, it is even
            if (f_array[i] > 50){ // if number is greater than 50, multiply by .78
                f_array[i] = f_array[i] * 0.78;
            }
            else{   //If less than 50, multiply by 1.04
                f_array[i] = f_array[i] * 1.04;
            }
        }
        else{   //If number is odd
            if (f_array[i] > 50){   // if number is greater than 50, multiply by .92
                f_array[i] = f_array[i] * 0.92;
            }
            else{   //If less than 50, multiply by 1.15
                f_array[i] = f_array[i] * 1.15;
            }
        }
    }

    /* Step  #3 Round all of the values to integers and assign them 
    to i_array using the round311 function */
    
    for (i=0; i<NUM_TEST_ELEMENTS; i++){
        i_array[i] = round311(f_array[i]);
    }
    
     /*Step #4 - Print out all of the floating point numbers in the 
    array in order on a SINGLE line.  Each value should be printed 
    with 2 digits to the right of the decimal point. */
    printf( "Testing printfloatArr311 (floats): " );
    printfloatArr311( f_array, NUM_TEST_ELEMENTS );
    printf("\n\n");
        
     /* Step #5 - Print out all of the integer numbers in the array in order on a SINGLE line.*/
    printf( "Testing printIntArr311 (integers): " );
    printIntArr311( i_array, NUM_TEST_ELEMENTS );
    printf("\n\n");

    /* Step #6 - sort the integer values, print values */
    printf( "Testing bubbleSort311 (integers): " );
    bubbleSort311( i_array, NUM_TEST_ELEMENTS );
    printIntArr311( i_array, NUM_TEST_ELEMENTS );
    printf("\n\n");

    /* Step #7 - print out the last 5 values of the integer array in binary. */
    printf( "Testing toBinary:\n" );
    for (i=NUM_TEST_ELEMENTS-6; i<NUM_TEST_ELEMENTS; i++) {
        toBinary(i_array[i]);
    }
    printf("\n\n");
    

    /* Declare an array of integers.  Fill the array with a count (times three) of the number of values for each 
    set of tens within the floating point array, i.e. index 0 will contain the count of rounded values in the array 0-9 TIMES THREE, the
    second will be 10-19, etc. */
    int min = 0;
    int max = 9;
    // find the count of elements of f_array in each set of 10 from 0 to 100.
    // Loop through 0-10 then add 10 each time to get count in 10-19, 20-29, etc.
    //Add count from 0-9 to hist_array index 0, 10-19 to index 1, etc
    for (i=0; i <10; i++){  
        hist_array[i] = countRange311(f_array, NUM_TEST_ELEMENTS, min, max);
        min += 10;  
        max += 10;
    }

   
     
    histogram311(hist_array);
    
    /* Exit the program successfully */
    printf( "\n\nCMPSC311 - Assignment #1 - Spring 2020 Complete.\n" );
    return( 0 );
}



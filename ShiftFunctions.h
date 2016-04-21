/*
 * ShiftFunctions.h
 *
 *  Created on: Apr 10, 2016
 *      Author: trotterkt
 */

#ifndef SHIFTFUNCTIONS_H_
#define SHIFTFUNCTIONS_H_
#include <sys/types.h>


// Taken from the CUDA implementation
//***************************************************************
void shiftRight(unsigned char* array, unsigned int bitSize, unsigned int arrayBitShift);

void shiftLeft(unsigned char* array, unsigned int bitSize, unsigned int arrayBitShift);
//***************************************************************



#endif /* SHIFTFUNCTIONS_H_ */

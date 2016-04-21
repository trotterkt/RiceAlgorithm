/*
 * SplitFunctions.cpp
 *
 *  Created on: Apr 10, 2016
 *      Author: trotterkt
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ShiftFunctions.h>
#include <AdaptiveEntropyEncoder.h>

using namespace RiceAlgorithm;

// Taken from the CUDA implementation
//***************************************************************
void shiftRight(unsigned char* array, unsigned int bitSize, unsigned int arrayBitShift)
{
    unsigned int numberOfBytes(bitSize/BitsPerByte);

    if(bitSize % BitsPerByte)
    {
        numberOfBytes++;
    }

    // Decide where in the copy the new bytes will go
    //unsigned char* arrayCopy = new unsigned char[numberOfBytes];
    // Not allocating from global memory is significantly faster
    const int MaximumByteArray(80);
    unsigned char arrayCopy[MaximumByteArray] = {0};

    // Shift from bit to bit, and byte to byte
    unsigned int byteShift = arrayBitShift / BitsPerByte;
    unsigned int bitShift = arrayBitShift % BitsPerByte;

    // Include neighboring bits to transfer to next byte
    // First figure out the mask
    unsigned char mask = powf(2, bitShift) - 1;
    unsigned char previousBits(0);


    // Copy from byte to shifted byte
    for(unsigned int byteIndex=0; byteIndex<numberOfBytes; byteIndex++)
    {
        // don't shift larger than the size of the stream
        if((byteIndex + byteShift) >= numberOfBytes)
        {
            break;
        }

        //***************************************************
        // do some index checking
        if((byteIndex + byteShift) >= MaximumByteArray)
        {
            printf("We have an error  in shiftRight-- (byteIndex + byteShift)=%d\n", (byteIndex + byteShift));
            return;
        }
        //***************************************************

        arrayCopy[byteIndex + byteShift] = (array[byteIndex]) >> bitShift;

        if (byteIndex > 0)
        {
            arrayCopy[byteIndex + byteShift] |= previousBits;
        }

        previousBits = (array[byteIndex] & mask) << (BitsPerByte - bitShift);
    }

    //***************************************************
    // do more index checking
    if((numberOfBytes) >= MaximumByteArray)
    {
        printf("We have an error  in shiftRight-- (numberOfBytes)=%d\n", numberOfBytes);
        return;
    }
    //***************************************************

    memcpy(array, arrayCopy, numberOfBytes);

}

void shiftLeft(unsigned char* array, unsigned int bitSize, unsigned int arrayBitShift)
{
    unsigned int numberOfBytes(bitSize/BitsPerByte);

    if(bitSize % BitsPerByte)
    {
        numberOfBytes++;
    }

    // Decide where in the copy the new bytes will go
    //unsigned char* arrayCopy = new unsigned char[numberOfBytes];
    // Not allocating from global memory is significantly faster
    const int MaximumByteArray(80);
    unsigned char arrayCopy[MaximumByteArray] = {0};

    memset(arrayCopy, 0, sizeof(arrayCopy));

    // Shift from bit to bit, and byte to byte
    unsigned int byteShift = arrayBitShift / BitsPerByte;
    unsigned int bitShift = arrayBitShift % BitsPerByte;

    // Include neighboring bits to transfer to next byte
    // First figure out the mask
    unsigned char mask = powf(2, bitShift) - 1;
    unsigned char previousBits(0);


    // Copy from byte to shifted byte
    for(unsigned int byteIndex=byteShift; byteIndex<numberOfBytes; byteIndex++)
    {
        // don't shift larger than the size of the stream
        if((byteIndex - byteShift) < 0)
        {
            break;
        }

        previousBits = (array[byteIndex+1] & (mask << (BitsPerByte - bitShift)));
        previousBits >>= (BitsPerByte - bitShift);

        //***************************************************
        // do some index checking
        if((byteIndex - byteShift) >= MaximumByteArray)
        {
            printf("We have an error  in shiftLeft -- (byteIndex - byteShift)=%d\n", (byteIndex + byteShift));
            return;
        }
        //***************************************************

        arrayCopy[byteIndex - byteShift] = (array[byteIndex]) << bitShift;

        if (byteIndex <= (numberOfBytes-1))
        {
            arrayCopy[byteIndex - byteShift] |= previousBits;
        }

    }

    //***************************************************
    // do more index checking
    if((numberOfBytes) >= MaximumByteArray)
    {
        printf("We have an error in shiftLeft -- (numberOfBytes)=%d\n", numberOfBytes);
        return;
    }
    //***************************************************

    memcpy(array, arrayCopy, numberOfBytes);
}
//***************************************************************





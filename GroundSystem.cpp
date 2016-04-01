/*
 * GroundSystem.cpp
 *
 *  Created on: Mar 28, 2016
 *      Author: trotterkt
 */

#include <GroundSystem.h>
#include <Sensor.h>
#include <Endian.h>
#include <iostream>
#include <stdio.h>
#include <math.h>

using namespace std;
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



GroundSystem::GroundSystem(ImagePersistence* image) : mySource(image), myRawSamples(0)
{
    memset(&myHeader, 0, sizeof(CompressedHeader));
}

GroundSystem::~GroundSystem()
{
    if(myRawSamples)
    {
        delete [] myRawSamples;
    }
}

void GroundSystem::process()
{
    // :TODO: This begins the decoding
    readHeader();

    // Having the raw sample dimensions from the header, allocate space for
    // the decoding
    const ulong NumberofSamples(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension);

    myRawSamples = new ushort[NumberofSamples];

    // Encoded data should begin right after the header (byte 19)

    // 1st grab the Encoded ID
    const int HeaderLength(19);
    unsigned int currentByteLocation(HeaderLength);
    unsigned int additionalBits(0);
    
    ushort* encodedBlockSizes = new ushort[(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension) / 32];

    // Read in one 32-sample block at a time (not on byte boundary)
    //for (long blockIndex = 0; blockIndex < NumberofSamples/32; blockIndex++)
    while (currentByteLocation < 1000) //:TODO: Temp
    {

        unsigned char selectionByteValue = mySource->getEncodedData()[currentByteLocation];
        unsigned char selectionByteValueNext = mySource->getEncodedData()[currentByteLocation+1];
        
        // Account for selection value not being on a byte boundary
        selectionByteValue <<= additionalBits;

        if(additionalBits > 4)
        {
            // In this case, we need the neighboring bits too
            selectionByteValue |= (selectionByteValueNext>>=additionalBits);
        }
        
        selectionByteValue >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);
        selectionByteValue -= 1;
        cout << "Encoding Selection = K" << int(selectionByteValue) << ", currentByteLocation=" << currentByteLocation << endl;
        CodingSelection selection = CodingSelection(selectionByteValue);

        // When the encoded zero-prefixed section ends, there should be a stream of 31 ones.
        // 1 binary bit for each of the 32 samples. This will be followed by 32 * k-select
        // value

        //:TRICKY:
        // count number ones in a byte
        //((i>>7)&1)+(i>>6)&1)+(i>>5)&1)+(i>>4)&1)+(i>>3)&1)+((i>>2)&1)+((i>>1)&1)+(i&1);

        // How do I pull out the encoded block lengths? See Sec. 6 of standard for CIP
        // alt. use 'a priori'


        // Assuming k-split type -
        // - count bits until 32-ones have been counted
        int encodeCount(0);
        unsigned char encodedByte = mySource->getEncodedData()[currentByteLocation];

        // Account for encoded value not being on a byte boundary
        const unsigned int CopySize(32 * sizeof(ushort));
        unsigned char encodedDataCopy[CopySize];
        memcpy(encodedDataCopy, &mySource->getEncodedData()[currentByteLocation], CopySize);

        shiftLeft(encodedDataCopy, CopySize*BitsPerByte, (CodeOptionBitFieldFundamentalOrNoComp+additionalBits));

        size_t encodedLength(0);

        ushort splitValue[32];
        int splitCount(0);
        int index(0);
        int copyIndex(0);
        
        int shiftPosition(7);

        while (encodeCount < 32)
        {
            // Count the bit if its '1'
            encodeCount += ((encodedDataCopy[copyIndex] >> (shiftPosition)) & 1);

            splitCount++;

            // Capture the encoded value
            //=====================================================

            if ((encodedDataCopy[copyIndex] >> (shiftPosition)) & 1)
            {
                splitValue[index] = splitCount;
                cout << "\nencodedSizeList[" << index << "]=" << splitValue[index] << endl;
                index++;
                splitCount = 0;
            }

            shiftPosition--;
            encodedLength++;

            if (shiftPosition < 0)
            {
                currentByteLocation++;
                copyIndex++;
                shiftPosition = BitsPerByte - 1;
            }
        }

        
        encodedLength += CodeOptionBitFieldFundamentalOrNoComp;

        // Total encoded length will be the current bit count, plus 32 x k-split
        cout << "\nencodedLength=" << encodedLength << endl;
        encodedLength += (32 * selection);
        cout << "\nencodedLength=" << encodedLength << endl;

        currentByteLocation += (32 * selection/BitsPerByte);
        cout << "currentByteLocation=" << currentByteLocation << endl;
        additionalBits += encodedLength%BitsPerByte;
        if(additionalBits > 7)
        {
            additionalBits = 0;
        }
        cout << "additional bits=" << additionalBits << endl;

        currentByteLocation++;
    }

    delete[] encodedBlockSizes;
}


void GroundSystem::readHeader()
{
    // Note: Header is not completely populated for all defined parameters.
    // Only what is applicable to the selected test raw data to
    // identify identical information.

    // Since the image written to the encoded file has already
    // been materialized, just read this directly rather than
    // re-reading from the file. Just trying to perform consecutive
    // encoding, decoding here
    char* encodedData = mySource->getEncodedData();

    memcpy(&myHeader.xDimension, &encodedData[1], sizeof(myHeader.xDimension));
    memcpy(&myHeader.yDimension, &encodedData[3], sizeof(myHeader.yDimension));
    memcpy(&myHeader.zDimension, &encodedData[5], sizeof(myHeader.zDimension));
    bigEndianVersusLittleEndian(myHeader.xDimension);
    bigEndianVersusLittleEndian(myHeader.yDimension);
    bigEndianVersusLittleEndian(myHeader.zDimension);

}

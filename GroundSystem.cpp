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

using namespace std;

namespace RiceAlgorithm
{

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
    myRawSamples = new ushort[myHeader.xDimension * myHeader.yDimension * myHeader.zDimension];


    // Encoded data should begin right after the header (byte 19)

    // 1st grab the Encoded ID
    const int HeaderLength(19);
    unsigned int currentByteLocation(HeaderLength);

    unsigned char selectionByteValue = mySource->getEncodedData()[currentByteLocation];
    selectionByteValue >>= (BitsPerByte - CodeOptionBitFieldFundamentalOrNoComp);
    selectionByteValue -= 1;
    CodingSelection selection = CodingSelection(selectionByteValue);
    
   
    // When the encoded zero-prefixed section ends, there should be a stream of 31 ones.
    // 1 binary bit for each of the 32 samples. This will be followed by 32 * k-select
    // value
    
    //:TRICKY:
    // count number ones in a byte
    //((i>>7)&1)+(i>>6)&1)+(i>>5)&1)+(i>>4)&1)+(i>>3)&1)+((i>>2)&1)+((i>>1)&1)+(i&1);
    
    // How do I pull out the encoded block lengths? See Sec. 6 of standard for CIP
    // alt. use 'a priori'
    
    ushort* encodedBlockSizes = new ushort[(myHeader.xDimension * myHeader.yDimension * myHeader.zDimension)/32];

    // Assuming k-split type -
    // - count bits until 32-ones have been counted
    int encodeCount(0);
    unsigned char encodedByte = mySource->getEncodedData()[currentByteLocation];

    size_t encodedLength(0);

   
    ushort splitValue[32];
    int splitCount(0);
    int index(0);

    int shiftPosition = CodeOptionBitFieldFundamentalOrNoComp-1;

    while(encodeCount < 32)
    {
        // Count the bit if its '1'
        encodeCount += ((encodedByte>>(shiftPosition))&1);
        
        splitCount++;

        // Capture the encoded value 
        //=====================================================

        if((encodedByte>>(shiftPosition))&1)
        {
            splitValue[index] = splitCount;
            cout << "\nencodedSizeList[" << index << "]=" << splitValue[index] << endl;
            index++;
            splitCount = 0;
        }
      
        shiftPosition--;
        encodedLength++;
        
        if(shiftPosition < 0)
        {
            currentByteLocation++;
            encodedByte = mySource->getEncodedData()[currentByteLocation];
            shiftPosition = BitsPerByte-1;
        }
    }

    encodedLength += CodeOptionBitFieldFundamentalOrNoComp;

    // Total encoded length will be the current bit count, plus 32 x k-split
    cout << "\nencodedLength=" << encodedLength << endl;
    encodedLength += (32 * selection);
    cout << "\nencodedLength=" << encodedLength << endl;

    delete [] encodedBlockSizes;
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

} /* namespace RiceAlgorithm */
